/*
 * (C) 2006-2014 see Authors.txt
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
#include "../../../DSUtil/AudioParser.h"
#include "WAVFile.h"

//
// CWAVFile
//

CWAVFile::CWAVFile()
	: CAudioFile()
	, m_length(0)
	, m_fmtdata(NULL)
	, m_fmtsize(0)
	, m_nBlockAlign(0)
	, m_blocksize(0)
{
}

CWAVFile::~CWAVFile()
{
	SAFE_DELETE_ARRAY(m_fmtdata);
}

bool CWAVFile::ProcessWAVEFORMATEX()
{
	if (!m_fmtdata || m_fmtsize < sizeof(WAVEFORMATEX)) {
		return false;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_fmtdata;
	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && m_fmtsize >= sizeof(WAVEFORMATEXTENSIBLE)) {
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_fmtdata;
		m_subtype = wfex->SubFormat;
		if (CountBits(wfex->dwChannelMask) != wfex->Format.nChannels) {
			// fix incorrect dwChannelMask
			wfex->dwChannelMask = GetDefChannelMask(wfex->Format.nChannels);
		}
	} else {
		m_subtype = FOURCCMap(wfe->wFormatTag);
	}

	if (wfe->wFormatTag == WAVE_FORMAT_PCM && (wfe->nChannels > 2 || wfe->wBitsPerSample != 8 && wfe->wBitsPerSample != 16)
			|| wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && wfe->nChannels > 2) {
		// convert incorrect WAVEFORMATEX to WAVEFORMATEXTENSIBLE

		WAVEFORMATEXTENSIBLE* wfex = new WAVEFORMATEXTENSIBLE;
		wfex->Format.wFormatTag				= WAVE_FORMAT_EXTENSIBLE;
		wfex->Format.nChannels				= wfe->nChannels;
		wfex->Format.nSamplesPerSec			= wfe->nSamplesPerSec;
		wfex->Format.wBitsPerSample			= (wfe->wBitsPerSample + 7) & ~7;
		wfex->Format.nBlockAlign			= wfex->Format.nChannels * wfex->Format.wBitsPerSample / 8;
		wfex->Format.nAvgBytesPerSec		= wfex->Format.nSamplesPerSec * wfex->Format.nBlockAlign;
		wfex->Format.cbSize					= 22;
		wfex->Samples.wValidBitsPerSample	= wfe->wBitsPerSample;
		wfex->dwChannelMask					= GetDefChannelMask(wfe->nChannels);
		wfex->SubFormat						= m_subtype;

		delete[] m_fmtdata;

		m_fmtdata = (BYTE*)wfex;
		m_fmtsize = sizeof(WAVEFORMATEXTENSIBLE);
		wfe = (WAVEFORMATEX*)m_fmtdata;
	}

	m_samplerate		= wfe->nSamplesPerSec;
	m_bitdepth			= wfe->wBitsPerSample;
	m_channels			= wfe->nChannels;
	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && m_fmtsize >= sizeof(WAVEFORMATEXTENSIBLE)) {
		m_layout = ((WAVEFORMATEXTENSIBLE*)m_fmtdata)->dwChannelMask;
	} else {
		m_layout = GetDefChannelMask(wfe->nChannels);
	}
	m_nAvgBytesPerSec	= wfe->nAvgBytesPerSec;
	m_nBlockAlign		= wfe->nBlockAlign;

	m_blocksize			= max(m_nBlockAlign, m_nAvgBytesPerSec / 16); // 62.5 ms
	m_blocksize			-= m_blocksize % m_nBlockAlign;

	return true;
}

bool CWAVFile::SetMediaType(CMediaType& mt)
{
	if (!m_fmtdata || !m_fmtsize) {
		return false;
	}

	mt.majortype	= MEDIATYPE_Audio;
	mt.formattype	= FORMAT_WaveFormatEx;
	mt.subtype		= m_subtype;
	mt.SetSampleSize(m_blocksize);

	memcpy(mt.AllocFormatBuffer(m_fmtsize), m_fmtdata, m_fmtsize);

	return true;
}

HRESULT CWAVFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	BYTE data[12];
	if (FAILED(m_pFile->ByteRead(data, sizeof(data)))
			|| *(DWORD*)(data+0) != FCC('RIFF')
			|| *(DWORD*)(data+4) < (4 + 8 + sizeof(PCMWAVEFORMAT) + 8) // 'WAVE' + ('fmt ' + fmt.size) + sizeof(PCMWAVEFORMAT) + (data + data.size)
			|| *(DWORD*)(data+8) != FCC('WAVE')) {
		return NULL;
	}
	__int64 end = min((__int64)*(DWORD*)(data + 4) + 8, m_pFile->GetLength());

#pragma pack(push, 4)
	union {
		struct {
			DWORD id;
			DWORD size;
		};
		BYTE data[8];
	} Chunk;
#pragma pack(pop)

	while (SUCCEEDED(m_pFile->ByteRead(Chunk.data, sizeof(Chunk))) && Chunk.id != FCC('data') && m_pFile->GetPos() < end) {
		__int64 pos = m_pFile->GetPos();

		TRACE(L"CWAVFile::Open() : found '%c%c%c%c' chunk.\n",
			TCHAR((Chunk.id>>0)&0xff),
			TCHAR((Chunk.id>>8)&0xff),
			TCHAR((Chunk.id>>16)&0xff),
			TCHAR((Chunk.id>>24)&0xff));

		switch (Chunk.id) {
		case FCC('fmt '):
			if (m_fmtdata || Chunk.size < sizeof(PCMWAVEFORMAT) || Chunk.size > 65536) {
				TRACE(L"CWAVFile::Open() : bad format\n");
				return E_FAIL;
			}
			m_fmtsize = max(Chunk.size, sizeof(WAVEFORMATEX)); // PCMWAVEFORMAT to WAVEFORMATEX
			m_fmtdata = new BYTE[m_fmtsize];
			memset(m_fmtdata, 0, m_fmtsize);
			if (FAILED(m_pFile->ByteRead(m_fmtdata, Chunk.size))) {
				TRACE(L"CWAVFile::Open() : format can not be read.\n");
				return E_FAIL;
			}
			break;
		case FCC('wavl'): // not supported
		case FCC('slnt'): // not supported
			TRACE(L"CWAVFile::Open() : WAVE file is not supported!\n");
			return E_FAIL;
		default:
			for (int i = 0; i < sizeof(Chunk.id); i++) {
				BYTE ch = Chunk.data[i];
				if (ch != 0x20 && (ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z')) {
					TRACE(L"CWAVFile::Open() : broken file!\n");
					return E_FAIL;
				}
			}
			// Known chunks:
			// 'fact'
			// 'LIST'
			// 'JUNK'
			// 'PAD '
			// 'cue '
			// 'plst'
			// 'list' (contains 'labl', 'note' and 'ltxt' subchunks)
			// 'PEAK'
			// 'smpl'
			// 'inst'
			// 'bext'
			// 'minf'
			// 'elm1'
			break;
		}

		m_pFile->Seek(pos + Chunk.size);
	}

	if (Chunk.id != FCC('data') || !ProcessWAVEFORMATEX()) {
		return E_FAIL;
	}

	m_startpos		= m_pFile->GetPos();
	m_length		= min(Chunk.size, m_pFile->GetLength() - m_startpos);
	m_length		-= m_length % m_nBlockAlign;
	m_endpos		= m_startpos + m_length;
	m_rtduration	= 10000000i64 * m_length / m_nAvgBytesPerSec;

	return S_OK;
}

REFERENCE_TIME CWAVFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	__int64 len = m_length * rt / m_rtduration;
	len -= len % m_nBlockAlign;
	m_pFile->Seek(m_startpos + len);

	rt = 10000000i64 * len / m_nAvgBytesPerSec;

	return rt;
}

int CWAVFile::GetAudioFrame(Packet* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->GetPos() + m_nBlockAlign > m_endpos) {
		return 0;
	}
	int size = min(m_blocksize, m_endpos - m_pFile->GetPos());
	packet->SetCount(size);
	m_pFile->ByteRead(packet->GetData(), size);

	__int64 len = m_pFile->GetPos() - m_startpos;
	packet->rtStart	= m_rtduration * len / m_length;
	packet->rtStop	= m_rtduration * (len + size) / m_length;

	return size;
}
