/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 * This file is part of MPC-BE.
 * YOU CANNOT USE THIS FILE WITHOUT AUTHOR PERMISSION!
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "mplayerc.h"
#include "SaveTaskDlg.h"
#include "../../filters/Filters.h"

// CSaveTaskDlg dialog

IMPLEMENT_DYNAMIC(CSaveTaskDlg, CTaskDialog)
CSaveTaskDlg::CSaveTaskDlg(CString in, CString out)
	: CTaskDialog(_T(""), /*_T("")*/in + _T("\n") + out, ResStr(IDS_SAVE_FILE), TDCBF_CANCEL_BUTTON, TDF_CALLBACK_TIMER|TDF_POSITION_RELATIVE_TO_WINDOW)
	, m_in(in), m_out(out)
	, m_TaskDlgHwnd(0)
{
	m_hIcon = (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon != NULL) {
		SetMainIcon(m_hIcon);
	}
	
	SetProgressBarMarquee();
	SetProgressBarRange(0, 100);
	SetProgressBarPosition(0);
	
	SetDialogWidth(220);

	InitFileCopy();
}

CSaveTaskDlg::~CSaveTaskDlg()
{
	if (pMC) {
		pMC->Stop();
	}

	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

HRESULT CSaveTaskDlg::OnInit()
{
	m_TaskDlgHwnd = ::GetActiveWindow();
	return S_OK;
}

HRESULT CSaveTaskDlg::InitFileCopy()
{
	if (FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)) || !(pMC = pGB) || !(pME = pGB) || !(pMS = pGB)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));
		return S_FALSE;
	}

	HRESULT hr;

	CStringW fnw = m_in;
	CComPtr<IFileSourceFilter> pReader;

	if (!pReader && m_in.Mid(m_in.ReverseFind('.')+1).MakeLower() == _T("cda")) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDDAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CCDXAReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader ) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)DNew CVTSReader(NULL, &hr);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		} else {
			CPath pout(m_out);
			pout.RenameExtension(_T(".ifo"));
			CopyFile(m_in, pout, FALSE);
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_AsyncReader);

		if (FAILED(hr) || !(pReader = pUnk) || FAILED(pReader->Load(fnw, NULL))) {
			pReader.Release();
		}
	}

	if (!pReader) {
		hr = S_OK;
		CComPtr<IUnknown> pUnk;
		pUnk.CoCreateInstance(CLSID_URLReader);

		if (CComQIPtr<IBaseFilter> pSrc = pUnk) { // url reader has to be in the graph to load the file
			pGB->AddFilter(pSrc, fnw);

			if (FAILED(hr) || !(pReader = pUnk) || FAILED(hr = pReader->Load(fnw, NULL))) {
				pReader.Release();
				pGB->RemoveFilter(pSrc);
			}
		}
	}

	CComQIPtr<IBaseFilter> pSrc = pReader;

	if (FAILED(pGB->AddFilter(pSrc, fnw))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(_T("Sorry, can't save this file, press Cancel"));
		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pMid = DNew CStreamDriveThruFilter(NULL, &hr);

	if (FAILED(pGB->AddFilter(pMid, L"StreamDriveThru"))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	CComQIPtr<IBaseFilter> pDst;
	pDst.CoCreateInstance(CLSID_FileWriter);
	CComQIPtr<IFileSinkFilter2> pFSF = pDst;
	pFSF->SetFileName(CStringW(m_out), NULL);
	pFSF->SetMode(AM_FILE_OVERWRITE);

	if (FAILED(pGB->AddFilter(pDst, L"File Writer"))) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(ResStr(IDS_AG_ERROR));

		return S_FALSE;
	}

	hr = pGB->Connect(
			 GetFirstPin((pSrc), PINDIR_OUTPUT),
			 GetFirstPin((pMid), PINDIR_INPUT));

	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(_T("Error Connect pSrc / pMid"));

		return S_FALSE;
	}

	hr = pGB->Connect(
			 GetFirstPin((pMid), PINDIR_OUTPUT),
			 GetFirstPin((pDst), PINDIR_INPUT));
	if (FAILED(hr)) {
		SetFooterIcon(MAKEINTRESOURCE(IDI_ERROR));
		SetFooterText(_T("Error Connect pMid / pDst"));

		return S_FALSE;
	}

	pMS = pMid;

	pMC->Run();

	return S_OK;
}

static unsigned int AdaptUnit(double& val, size_t unitsNb)
{
	unsigned int unit = 0;

	while (val > 1024 && unit < unitsNb) {
		val /= 1024;
		unit++;
	}

	return unit;
}

HRESULT CSaveTaskDlg::OnTimer(_In_ long lTime)
{
	static UINT sizeUnits[]		= { IDS_SIZE_UNIT_K,	IDS_SIZE_UNIT_M,	IDS_SIZE_UNIT_G };
	static UINT speedUnits[]	= { IDS_SPEED_UNIT_K,	IDS_SPEED_UNIT_M,	IDS_SPEED_UNIT_G };

	if (pGB && pMS) {
		CString str;
		REFERENCE_TIME pos = 0, dur = 0;
		pMS->GetCurrentPosition(&pos);
		pMS->GetDuration(&dur);
		REFERENCE_TIME time = 0;
		CComQIPtr<IMediaSeeking>(pGB)->GetCurrentPosition(&time);
		REFERENCE_TIME speed = time > 0 ? pos*10000000/time : 0;

		double dPos = pos / 1024.;
		unsigned int unitPos = AdaptUnit(dPos, _countof(sizeUnits));
		double dDur = dur / 1024.;
		unsigned int unitDur = AdaptUnit(dDur, _countof(sizeUnits));
		double dSpeed = speed / 1024.;
		unsigned int unitSpeed = AdaptUnit(dSpeed, _countof(speedUnits));

		str.Format(_T("%.2lf %s / %.2lf %s , %.2lf %s, %I64d s"),
				   dPos, ResStr(sizeUnits[unitPos]), dDur, ResStr(sizeUnits[unitDur]),
				   dSpeed, ResStr(speedUnits[unitSpeed]), speed > 0 ? (dur-pos) / speed : 0);
		SetContent(str);

		SetProgressBarPosition(dur > 0 ? (int)(100*pos/dur) : 0);

		if (dur && pos >= dur) {
			::SendMessage(m_TaskDlgHwnd, TDM_CLICK_BUTTON, static_cast<WPARAM>(TDCBF_CANCEL_BUTTON), 0);
			
			return S_FALSE;
		}
	}

	return S_OK;
}