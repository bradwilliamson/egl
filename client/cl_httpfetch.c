/*
Copyright (C) 20-2026 EGL Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#include "cl_local.h"
#include "cl_httpfetch.h"

static void CL_HTTPFetch_SetErr(char *errBuf, size_t errBufSize, const char *fmt, ...)
{
	va_list argptr;

	if (!errBuf || !errBufSize)
		return;
	errBuf[0] = 0;
	va_start(argptr, fmt);
	vsnprintf(errBuf, errBufSize, fmt, argptr);
	errBuf[errBufSize - 1] = 0;
	va_end(argptr);
}

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

static qBool CL_HTTPFetch_WinHTTP_DoOnce(const wchar_t *host, INTERNET_PORT port, const wchar_t *path, qBool secure,
	byte **outData, size_t *outLen, wchar_t *redirectBuf, size_t redirectChars, char *errBuf, size_t errBufSize)
{
	HINTERNET session = NULL;
	HINTERNET connect = NULL;
	HINTERNET request = NULL;
	DWORD flags = 0;
	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	byte *buffer = NULL;
	size_t cap = 0;
	size_t len = 0;
	qBool ok = qFalse;
	byte *newBuf;

	*redirectBuf = 0;
	*outData = NULL;
	*outLen = 0;

	session = WinHttpOpen(L"EGL/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpOpen failed (%lu)", GetLastError());
		goto done;
	}

	// Reasonable timeouts for a manual refresh.
	WinHttpSetTimeouts(session, 3000, 3000, 5000, 5000);

	connect = WinHttpConnect(session, host, port, 0);
	if (!connect) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpConnect failed (%lu)", GetLastError());
		goto done;
	}

	if (secure)
		flags |= WINHTTP_FLAG_SECURE;

	request = WinHttpOpenRequest(connect, L"GET", path, NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!request) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpOpenRequest failed (%lu)", GetLastError());
		goto done;
	}

	// Avoid compression to keep parsing simple.
	(void)WinHttpAddRequestHeaders(request,
		L"Accept-Encoding: identity\r\n"
		L"Cache-Control: no-cache\r\n"
		L"Pragma: no-cache\r\n",
		(DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

	if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpSendRequest failed (%lu)", GetLastError());
		goto done;
	}

	if (!WinHttpReceiveResponse(request, NULL)) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpReceiveResponse failed (%lu)", GetLastError());
		goto done;
	}

	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpQueryHeaders(status) failed (%lu)", GetLastError());
		goto done;
	}

	if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
		DWORD need = 0;
		WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &need, WINHTTP_NO_HEADER_INDEX);
		if (need > 0 && need / sizeof(wchar_t) < redirectChars) {
			DWORD bytes = need;
			if (WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
				redirectBuf, &bytes, WINHTTP_NO_HEADER_INDEX)) {
				redirectBuf[redirectChars - 1] = 0;
				ok = qTrue; // signal redirect
				goto done;
			}
		}
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "HTTP redirect (%lu) without usable Location", (unsigned long)status);
		goto done;
	}

	if (status != 200) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "HTTP status %lu", (unsigned long)status);
		goto done;
	}

	for (;;) {
		DWORD avail = 0;
		DWORD read = 0;
		if (!WinHttpQueryDataAvailable(request, &avail)) {
			CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpQueryDataAvailable failed (%lu)", GetLastError());
			goto done;
		}
		if (!avail)
			break;

		if (len + avail + 1 > cap) {
			size_t newCap = cap ? cap : 4096;
			while (newCap < len + avail + 1)
				newCap *= 2;
			newBuf = Mem_Alloc(newCap);
			if (buffer && cap)
				memcpy(newBuf, buffer, cap);
			if (buffer)
				Mem_Free(buffer);
			buffer = newBuf;
			cap = newCap;
		}

		if (!WinHttpReadData(request, buffer + len, avail, &read)) {
			CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpReadData failed (%lu)", GetLastError());
			goto done;
		}
		len += read;
		if (!read)
			break;
	}

	if (!buffer) {
		buffer = Mem_Alloc(1);
		buffer[0] = 0;
		len = 0;
		cap = 1;
	}
	buffer[len] = 0;

	*outData = buffer;
	*outLen = len;
	buffer = NULL;
	ok = qTrue;

done:
	if (request)
		WinHttpCloseHandle(request);
	if (connect)
		WinHttpCloseHandle(connect);
	if (session)
		WinHttpCloseHandle(session);
	if (buffer)
		Mem_Free(buffer);
	return ok;
}

static qBool CL_HTTPFetch_WinHTTP(const char *url, byte **outData, size_t *outLen, char *errBuf, size_t errBufSize)
{
	URL_COMPONENTS uc;
	wchar_t wurl[2048];
	wchar_t host[256];
	wchar_t path[1536];
	wchar_t scheme[32];
	wchar_t redirect[2048];
	DWORD wlen;
	int redirects = 0;

	*outData = NULL;
	*outLen = 0;

	if (!url || !url[0]) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "Empty URL");
		return qFalse;
	}

	// UTF-8 -> UTF-16 (ACP fallback)
	wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, (int)ARRAYSIZE(wurl));
	if (!wlen)
		wlen = MultiByteToWideChar(CP_ACP, 0, url, -1, wurl, (int)ARRAYSIZE(wurl));
	if (!wlen) {
		CL_HTTPFetch_SetErr(errBuf, errBufSize, "URL conversion failed");
		return qFalse;
	}

	for (;;) {
		memset(&uc, 0, sizeof(uc));
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = host;
		uc.dwHostNameLength = (DWORD)ARRAYSIZE(host);
		uc.lpszUrlPath = path;
		uc.dwUrlPathLength = (DWORD)ARRAYSIZE(path);
		uc.lpszScheme = scheme;
		uc.dwSchemeLength = (DWORD)ARRAYSIZE(scheme);

		if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) {
			CL_HTTPFetch_SetErr(errBuf, errBufSize, "WinHttpCrackUrl failed (%lu)", GetLastError());
			return qFalse;
		}

		host[ARRAYSIZE(host) - 1] = 0;
		path[ARRAYSIZE(path) - 1] = 0;
		scheme[ARRAYSIZE(scheme) - 1] = 0;

		{
			qBool secure = (uc.nScheme == INTERNET_SCHEME_HTTPS);
			INTERNET_PORT port = uc.nPort;
			qBool ok = CL_HTTPFetch_WinHTTP_DoOnce(host, port, path, secure, outData, outLen,
				redirect, ARRAYSIZE(redirect), errBuf, errBufSize);
			if (!ok)
				return qFalse;
			if (!redirect[0])
				return qTrue;
		}

		// Follow redirect.
		if (++redirects > 3) {
			CL_HTTPFetch_SetErr(errBuf, errBufSize, "Too many redirects");
			return qFalse;
		}
		wcsncpy(wurl, redirect, ARRAYSIZE(wurl) - 1);
		wurl[ARRAYSIZE(wurl) - 1] = 0;
	}
}

#endif // _WIN32

qBool CL_HTTPFetch(const char *url, byte **outData, size_t *outLen, char *errBuf, size_t errBufSize)
{
	if (outData)
		*outData = NULL;
	if (outLen)
		*outLen = 0;
	if (errBuf && errBufSize)
		errBuf[0] = 0;

#ifdef _WIN32
	return CL_HTTPFetch_WinHTTP(url, outData, outLen, errBuf, errBufSize);
#else
	CL_HTTPFetch_SetErr(errBuf, errBufSize, "HTTP fetch not supported on this platform/build");
	return qFalse;
#endif
}
