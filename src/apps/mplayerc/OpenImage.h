/*
 * $Id$
 *
 * Copyright (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

using namespace Gdiplus;

HBITMAP OpenImage(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (wcsstr(tmp_fn, L".bmp")
		|| wcsstr(tmp_fn, L".jpg")
		|| wcsstr(tmp_fn, L".jpeg")
		|| wcsstr(tmp_fn, L".png")
		|| wcsstr(tmp_fn, L".gif")
		|| wcsstr(tmp_fn, L".tif")
		|| wcsstr(tmp_fn, L".tiff")
		|| wcsstr(tmp_fn, L".emf")
		|| wcsstr(tmp_fn, L".ico")) {

		FILE *fp;
		TCHAR path_fn[_MAX_PATH];
		int type = 0;

		if (wcsstr(fn, L"://")) {
			HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);
			if (s) {
				f = InternetOpenUrlW(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);
				if (f) {
					type++;

					DWORD len;
					char buf[4096];
					TCHAR path[_MAX_PATH];

					GetTempPath(_MAX_PATH, path);
					GetTempFileName(path, _T("mpc_image"), 0, path_fn);
					fp = _tfopen(path_fn, _T("wb+"));

					for (;;) {
						InternetReadFile(f, buf, sizeof(buf), &len);

						if (!len) {
							break;
						}

						fwrite(buf, 1, len, fp);
					}
					InternetCloseHandle(f);
				}
				InternetCloseHandle(s);
			}
		} else {
			fp = _tfopen(fn, _T("rb"));
			fseek(fp, 0, SEEK_END);
		}

		DWORD fs = ftell(fp);
		rewind(fp);
		HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, fs);
		fread((void*)hG, 1, fs, fp);
		fclose(fp);

		if (type) {
			_tunlink(path_fn);
		}

		IStream *s;
		::CreateStreamOnHGlobal(hG, 1, &s);

		ULONG_PTR gdiplusToken;
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
		Bitmap *bm = Bitmap::FromStream(s);
		HBITMAP hB;
		bm->GetHBITMAP(0, &hB);
		delete bm;
		GdiplusShutdown(gdiplusToken);

		::GlobalFree(hG);
		s->Release();

		return hB;
	} else {
		return NULL;
	}
}