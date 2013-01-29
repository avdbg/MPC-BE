/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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
#include "mplayerc.h"
#include "PPageYoutube.h"
#include "PlayerYouTube.h"
#include "AppSettings.h"
//#include "ComPropertySheet.h"

// CPPageYoutube dialog

IMPLEMENT_DYNAMIC(CPPageYoutube, CPPageBase)
CPPageYoutube::CPPageYoutube()
	: CPPageBase(CPPageYoutube::IDD, CPPageYoutube::IDD)
	, m_iYoutubeFormatType(0)
{
}

CPPageYoutube::~CPPageYoutube()
{
}

void CPPageYoutube::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_iYoutubeFormatCtrl);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_iYoutubeFormatCtrl.AddString(ResStr(IDS_PPAGE_FS_DEFAULT));
	m_iYoutubeFormatType = 0;
	for (int i = 0; i < _countof(youtubeProfiles) && youtubeProfiles[i].iTag <= 46; i++) {
		CString fmt; fmt.Format(_T("%s@%s"), youtubeProfiles[i].Container, youtubeProfiles[i].Resolution);
		m_iYoutubeFormatCtrl.AddString(fmt);

		if (youtubeProfiles[i].iTag == s.iYoutubeTag) {
			m_iYoutubeFormatType = i + 1;
		}
	}
	CorrectComboListWidth(m_iYoutubeFormatCtrl);
	m_iYoutubeFormatCtrl.SetCurSel(m_iYoutubeFormatType);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	m_iYoutubeFormatType = m_iYoutubeFormatCtrl.GetCurSel();
	if (m_iYoutubeFormatType <= 0 || (m_iYoutubeFormatType - 1) >= _countof(youtubeProfiles)) {
		s.iYoutubeTag = m_iYoutubeFormatType;
	} else {
		s.iYoutubeTag = youtubeProfiles[m_iYoutubeFormatType - 1].iTag;
	}

	return __super::OnApply();
}