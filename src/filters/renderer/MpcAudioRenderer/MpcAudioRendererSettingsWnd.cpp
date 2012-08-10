/*
 * $Id$
 *
 * (C) 2010-2012 see Authors.txt
 *
 * This file is part of MPC-BE.
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
#include "MpcAudioRendererSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"

// ==>>> Resource identifier from "resource.h" present in mplayerc project!
#define ResStr(id) CString(MAKEINTRESOURCE(id))

#define LEFT_SPACING					25
#define VERTICAL_SPACING				25

CMpcAudioRendererSettingsWnd::CMpcAudioRendererSettingsWnd(void)
{
}

bool CMpcAudioRendererSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMAR);

	m_pMAR.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMAR = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMAR) {
		return false;
	}

	return true;
}

void CMpcAudioRendererSettingsWnd::OnDisconnect()
{
	m_pMAR.Release();
}

bool CALLBACK DSEnumProc(LPGUID lpGUID,
						 LPCTSTR lpszDesc,
						 LPCTSTR lpszDrvName,
						 LPVOID lpContext )
{
	CComboBox *pCombo = (CComboBox*)lpContext;
	ASSERT ( pCombo );
	LPGUID lpTemp = NULL;

	if (lpGUID != NULL) // NULL only for "Primary Sound Driver".
	{
		if ((lpTemp = (LPGUID)malloc(sizeof(GUID))) == NULL)
		{
			return TRUE;
		}
		memcpy(lpTemp, lpGUID, sizeof(GUID));
	}
	pCombo->AddString ( lpszDesc );
	free(lpTemp);
	return TRUE;
}

bool CMpcAudioRendererSettingsWnd::OnActivate()
{
	int nPosY	= 10;
	CSize wsize	= GetWindowSize();

	m_grpDefault.Create (_T(""), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  nPosY,  wsize.cx, nPosY+ wsize.cy), this, (UINT)IDC_STATIC);
	nPosY += VERTICAL_SPACING;
	m_cbWasapiMode.Create (ResStr (IDS_ARS_WASAPI_MODE), WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 325, nPosY+15), this, IDC_PP_WASAPI_MODE);

	nPosY += VERTICAL_SPACING;
	m_txtWasapiModeType.Create (_T("Wasapi mode"), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 100, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbWasapiModeType.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (110,  nPosY-4, 325, nPosY+90), this, IDC_PP_WASAPI_MODE_TYPE);
	m_cbWasapiModeType.AddString(_T("Exclusive Mode"));
	m_cbWasapiModeType.AddString(_T("Shared Mode"));
	// TODO - translate ...

	nPosY += VERTICAL_SPACING + 10;
	m_cbMuteFastForward.Create (ResStr (IDS_ARS_MUTE_FAST_FORWARD), WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 325, nPosY+15), this, IDC_PP_MUTE_FAST_FORWARD);
	nPosY += VERTICAL_SPACING;
	m_txtSoundDevice.Create (ResStr (IDS_ARS_SOUND_DEVICE), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 100, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbSoundDevice.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (110,  nPosY-4, 325, nPosY+90), this, IDC_PP_SOUND_DEVICE);

	SetClassLongPtr(GetDlgItem(IDC_PP_SOUND_DEVICE)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));

	DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc, (VOID*)&m_cbSoundDevice);

	if (m_pMAR) {
		if ( m_cbSoundDevice.GetCount() > 0 ) {
			int idx = m_cbSoundDevice.FindString(0, m_pMAR->GetSoundDevice());
			if ( idx < 0) {
				m_cbSoundDevice.SetCurSel(0);
			}
			else {
				m_cbSoundDevice.SetCurSel(m_cbSoundDevice.FindString(0, m_pMAR->GetSoundDevice()));
			}
		}

		CorrectComboListWidth(m_cbSoundDevice);

		m_cbWasapiMode.SetCheck(m_pMAR->GetWasapiMode());
		m_cbMuteFastForward.SetCheck(m_pMAR->GetMuteFastForward());

		m_cbWasapiModeType.SetCurSel(m_pMAR->GetWasapiModeType());
		// m_txtWasapiModeType.EnableWindow(m_cbWasapiMode.GetCheck());
		// m_cbWasapiModeType.EnableWindow(m_cbWasapiMode.GetCheck());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	return true;
}

void CMpcAudioRendererSettingsWnd::OnDeactivate()
{
}

bool CMpcAudioRendererSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMAR) {
		m_pMAR->SetWasapiMode(m_cbWasapiMode.GetCheck());
		m_pMAR->SetMuteFastForward(m_cbMuteFastForward.GetCheck());
		m_pMAR->SetWasapiModeType(m_cbWasapiModeType.GetCurSel());
		CString str;
		int idx = m_cbSoundDevice.GetCurSel();
		if ( !(idx < 0) ) {
			m_cbSoundDevice.GetLBText( idx, str );
			m_pMAR->SetSoundDevice(str);
		}
		m_pMAR->Apply();
	}

	return true;
}


BEGIN_MESSAGE_MAP(CMpcAudioRendererSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
