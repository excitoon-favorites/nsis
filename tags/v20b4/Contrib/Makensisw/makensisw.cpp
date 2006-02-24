/*
  Copyright (c) 2002 Robert Rainwater
  Contributors: Justin Frankel, Fritz Elfert, Amir Szekely, and Sunil Kamath

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include <windows.h>
#include <stdio.h>
#include "makensisw.h"
#include "resource.h"
#include "noclib.h"
#include "toolbar.h"

NSCRIPTDATA g_sdata;
NRESIZEDATA g_resize;
NFINDREPLACE g_find;
extern NTOOLBAR g_toolbar;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, char *cmdParam, int cmdShow) {
  MSG  msg;
  int status;
  HACCEL haccel;
  my_memset(&g_sdata,0,sizeof(NSCRIPTDATA));
  my_memset(&g_resize,0,sizeof(NRESIZEDATA));
  my_memset(&g_find,0,sizeof(NFINDREPLACE));
  g_sdata.hInstance=GetModuleHandle(0);
  g_sdata.script_alloced=false;
  g_sdata.defines = NULL;
  RestoreDefines();
  g_sdata.script=GetCommandLine();
  if (*g_sdata.script=='"') { g_sdata.script++; while (*g_sdata.script && *g_sdata.script++!='"' ); }
  else while (*g_sdata.script!=' ' && *g_sdata.script) g_sdata.script++;
  while (*g_sdata.script==' ') g_sdata.script++;
  if(lstrlen(g_sdata.script)) {
    bool is_quoted = false;
    char *p = g_sdata.script + (lstrlen(g_sdata.script) - 1);

    if(*p == '"') is_quoted = true;
    p--;
    while(p > g_sdata.script) {
      if(*p == ' ') {
        if(!is_quoted) {
          p++;
          break;
        }
      }
      else if(*p == '"') {
        p++;
        break;
      }
      p--;
    }
    PushMRUFile(p);
  }

  if (!InitBranding()) {
    MessageBox(0,NSISERROR,"Error",MB_ICONEXCLAMATION|MB_OK);
    return 1;
  }
  ResetObjects();
  HWND hDialog = CreateDialog(g_sdata.hInstance,MAKEINTRESOURCE(DLG_MAIN),0,DialogProc);
  if (!hDialog) {
    MessageBox(0,DLGERROR,"Error",MB_ICONEXCLAMATION|MB_OK);
    return 1;
  }
  haccel = LoadAccelerators(g_sdata.hInstance, MAKEINTRESOURCE(IDK_ACCEL));
  while ((status=GetMessage(&msg,0,0,0))!=0) {
    if (status==-1) return -1;
    if (!IsDialogMessage(g_find.hwndFind, &msg)) {
      if (!TranslateAccelerator(hDialog,haccel,&msg)) {
        if (!IsDialogMessage(hDialog,&msg)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }
  }
  if (g_sdata.script_alloced) GlobalFree(g_sdata.script);
  ExitProcess(msg.wParam);
  return msg.wParam;
}

void ResetInputScript()
{
    if(g_sdata.input_script) {
      g_sdata.script_alloced = true;
      g_sdata.script = (char *)GlobalAlloc(GPTR, (lstrlen(g_sdata.input_script)+3)*sizeof(char));
      wsprintf(g_sdata.script,"\"%s\"",g_sdata.input_script);
    }
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  static HINSTANCE hRichEditDLL = 0;
  if (!hRichEditDLL) hRichEditDLL= LoadLibrary("RichEd32.dll");
  switch (msg) {
    case WM_INITDIALOG:
    {
      g_sdata.hwnd=hwndDlg;
      HICON hIcon = LoadIcon(g_sdata.hInstance,MAKEINTRESOURCE(IDI_ICON));
      SetClassLong(hwndDlg,GCL_HICON,(long)hIcon);
      // Altered by Darren Owen (DrO) on 29/9/2003
      // Added in receiving of mouse and key events from the richedit control
      SendMessage(GetDlgItem(hwndDlg,IDC_LOGWIN),EM_SETEVENTMASK,NULL,ENM_SELCHANGE|ENM_MOUSEEVENTS|ENM_KEYEVENTS);
      DragAcceptFiles(g_sdata.hwnd,FALSE);
      g_sdata.menu = GetMenu(g_sdata.hwnd);
      g_sdata.fileSubmenu = GetSubMenu(g_sdata.menu, FILE_MENU_INDEX);
      g_sdata.editSubmenu = GetSubMenu(g_sdata.menu, EDIT_MENU_INDEX);
      g_sdata.toolsSubmenu = GetSubMenu(g_sdata.menu, TOOLS_MENU_INDEX);
      RestoreMRUList();
      CreateToolBar();
      InitTooltips(g_sdata.hwnd);
      SetBranding(g_sdata.hwnd);
      HFONT hFont = CreateFont(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH|FF_DONTCARE,"Courier New");
      SendDlgItemMessage(hwndDlg,IDC_LOGWIN,WM_SETFONT,(WPARAM)hFont,0);
      SendDlgItemMessage(hwndDlg,IDC_LOGWIN,EM_SETBKGNDCOLOR,0,GetSysColor(COLOR_BTNFACE));
      RestoreWindowPos(g_sdata.hwnd);
      RestoreCompressor();
      CompileNSISScript();
      return TRUE;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      GetClientRect(g_sdata.hwnd, &g_resize.griprect);
      HDC hdc = BeginPaint(g_sdata.hwnd, &ps);
      g_resize.griprect.left = g_resize.griprect.right - GetSystemMetrics(SM_CXVSCROLL);
      g_resize.griprect.top = g_resize.griprect.bottom - GetSystemMetrics(SM_CYVSCROLL);
      DrawFrameControl(hdc, &g_resize.griprect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
      EndPaint(g_sdata.hwnd,&ps);
      return TRUE;
    }
    case WM_DESTROY:
    {
      SaveDefines();
      SaveMRUList();
      SaveCompressor();
      SaveWindowPos(g_sdata.hwnd);
      DestroyTooltips();
      PostQuitMessage(0);
      return TRUE;
    }
    case WM_CLOSE:
    {
      if (!g_sdata.thread) {
        DragAcceptFiles(g_sdata.hwnd,FALSE);
        DestroyWindow(hwndDlg);
        FreeLibrary(hRichEditDLL);
      }
      return TRUE;
    }
    case WM_DROPFILES: {
      int num;
      char szTmp[MAX_PATH];
      num = DragQueryFile((HDROP)wParam,-1,NULL,0);
      if (num==1) {
        DragQueryFile((HDROP)wParam,0,szTmp,MAX_PATH);
        if (lstrlen(szTmp)>0) {
          g_sdata.script_alloced = true;
          g_sdata.script = (char *)GlobalAlloc(GPTR,sizeof(szTmp)+7);
          wsprintf(g_sdata.script,"\"%s\"",szTmp);
          PushMRUFile(g_sdata.script);
          ResetObjects();
          CompileNSISScript();
        }
      }
      break;
    }
    case WM_GETMINMAXINFO:
    {
      ((MINMAXINFO*)lParam)->ptMinTrackSize.x=MINWIDTH;
      ((MINMAXINFO*)lParam)->ptMinTrackSize.y=MINHEIGHT;
    }
    case WM_ENTERSIZEMOVE:
    {
      GetClientRect(g_sdata.hwnd, &g_resize.resizeRect);
      return TRUE;
    }
    case WM_SIZE:
    {
      if ((wParam == SIZE_MAXHIDE)||(wParam == SIZE_MAXSHOW)) return TRUE;
      RECT rSize;
      if (hwndDlg == g_sdata.hwnd) {
        GetClientRect(g_sdata.hwnd, &rSize);
        if (((rSize.right==0)&&(rSize.bottom==0))||((g_resize.resizeRect.right==0)&&(g_resize.resizeRect.bottom==0)))  return TRUE;
        g_resize.dx = rSize.right - g_resize.resizeRect.right;
        g_resize.dy = rSize.bottom - g_resize.resizeRect.bottom;
        EnumChildWindows(g_sdata.hwnd, DialogResize, (LPARAM)0);
        g_resize.resizeRect = rSize;
      }
      return TRUE;
    }
    case WM_SIZING:
    {
      InvalidateRect(g_sdata.hwnd,&g_resize.griprect,TRUE);
      GetClientRect(g_sdata.hwnd, &g_resize.griprect);
      g_resize.griprect.left = g_resize.griprect.right - GetSystemMetrics(SM_CXVSCROLL);
      g_resize.griprect.top = g_resize.griprect.bottom - GetSystemMetrics(SM_CYVSCROLL);
      return TRUE;
    }
    case WM_MAKENSIS_PROCESSCOMPLETE:
    {
      if (g_sdata.thread) {
        CloseHandle(g_sdata.thread);
        g_sdata.thread=0;
      }
      if(g_sdata.compressor == COMPRESSOR_BEST) {
        if (g_sdata.retcode==0 && FileExists(g_sdata.output_exe)) {
          char zlib_file_name[MAX_PATH];
          wsprintf(zlib_file_name,"%s_makensisw_zlib",g_sdata.output_exe);
          if(!lstrcmpi(g_sdata.compressor_name,ZLIB_COMPRESSOR_NAME)) {
            CopyFile(g_sdata.output_exe,zlib_file_name,false);
            g_sdata.compressor_name = BZIP2_COMPRESSOR_NAME;
            ResetObjects();
            ResetInputScript();

            CompileNSISScript();
            return TRUE;
          }
          else {
            g_sdata.compressor_name = ZLIB_COMPRESSOR_NAME;
            g_sdata.appended = false;
            ResetInputScript();

            if(FileExists(zlib_file_name)) {
              HANDLE hZlib, hBzip2;
              DWORD zlibSize, bzip2Size;

              hZlib = CreateFile(zlib_file_name,GENERIC_READ, FILE_SHARE_READ,
                                 NULL, OPEN_EXISTING, NULL, NULL);
              if(hZlib != INVALID_HANDLE_VALUE) {
                zlibSize = GetFileSize(hZlib, 0);
                CloseHandle(hZlib);

                if(zlibSize != INVALID_FILE_SIZE) {
                  hBzip2 = CreateFile(g_sdata.output_exe,GENERIC_READ, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, NULL, NULL);
                  if(hBzip2 != INVALID_HANDLE_VALUE) {
                    bzip2Size = GetFileSize(hBzip2, 0);
                    CloseHandle(hBzip2);

                    char buf[1024];

                    if(bzip2Size != INVALID_FILE_SIZE) {
                      if(zlibSize < bzip2Size) {
                        CopyFile(zlib_file_name,g_sdata.output_exe,false);
                        wsprintf(buf,COMPRESSOR_MESSAGE,ZLIB_COMPRESSOR_NAME,zlibSize,
                                 BZIP2_COMPRESSOR_NAME,bzip2Size);
                        LogMessage(g_sdata.hwnd,buf);
                        LogMessage(g_sdata.hwnd,ZLIB_COMPRESSOR_MESSAGE);
                        LogMessage(g_sdata.hwnd, g_sdata.compressor_stats);
                      }
                      else {
                        wsprintf(buf,COMPRESSOR_MESSAGE,BZIP2_COMPRESSOR_NAME,bzip2Size,
                                 ZLIB_COMPRESSOR_NAME,zlibSize);
                        LogMessage(g_sdata.hwnd,buf);
                      }
                    }
                  }
                }
              }
              DeleteFile(zlib_file_name);
            }
          }
        }
      }
      EnableItems(g_sdata.hwnd);
      if (g_sdata.retcode==0) {
        MessageBeep(MB_ICONASTERISK);
        if (g_sdata.warnings) SetTitle(g_sdata.hwnd,"Finished with Warnings");
        else {
          SetTitle(g_sdata.hwnd,"Finished Sucessfully");
          // Added by Darren Owen (DrO) on 1/10/2003
          if(g_sdata.recompile_test)
            PostMessage(g_sdata.hwnd, WM_COMMAND, LOWORD(IDM_TEST), 0);
        }
      }
      else {
        MessageBeep(MB_ICONEXCLAMATION);
        SetTitle(g_sdata.hwnd,"Compile Error: See Log for Details");
      }

      // Added by Darren Owen (DrO) on 1/10/2003
      // ensures the recompile and run state is reset after use
      g_sdata.recompile_test = 0;
      DragAcceptFiles(g_sdata.hwnd,TRUE);
      return TRUE;
    }
    case WM_NOTIFY:
      switch (((NMHDR*)lParam)->code ) {
        case EN_SELCHANGE:
          SendDlgItemMessage(hwndDlg,IDC_LOGWIN, EM_EXGETSEL, 0, (LPARAM) &g_sdata.textrange);
          {
            BOOL enabled = (g_sdata.textrange.cpMax-g_sdata.textrange.cpMin<=0?FALSE:TRUE);
            EnableMenuItem(g_sdata.menu,IDM_COPYSELECTED,(enabled?MF_ENABLED:MF_GRAYED));
            EnableToolBarButton(IDM_COPY,enabled);
          }
        // Altered by Darren Owen (DrO) on 6/10/2003
        // Allows the detection of the right-click menu when running on OSes below Windows 2000
        // and will then simulate the effective WM_CONTEXTMENU message that would be received
        // note: removed the WM_CONTEXTMENU handling to prevent a duplicate menu appearing on
        // Windows 2000 and higher
        case EN_MSGFILTER:
          #define lpnmMsg ((MSGFILTER*)lParam)
          if(WM_RBUTTONUP == lpnmMsg->msg || WM_KEYUP == lpnmMsg->msg && lpnmMsg->wParam == VK_APPS){
          POINT pt;
          HWND edit = GetDlgItem(g_sdata.hwnd,IDC_LOGWIN);
          RECT r;
            GetCursorPos(&pt);

            // Added and altered by Darren Owen (DrO) on 29/9/2003
            // Will place the right-click menu in the top left corner of the window
            // if the application key is pressed and the mouse is not in the window
            // from here...
            ScreenToClient(edit, &pt);
            GetClientRect(edit, &r);
            if(!PtInRect(&r, pt))
              pt.x = pt.y = 0;
            MapWindowPoints(edit, HWND_DESKTOP, &pt, 1);
            TrackPopupMenu(g_sdata.editSubmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_sdata.hwnd, 0);
          }
        case TBN_DROPDOWN:
        {
          LPNMTOOLBAR pToolBar = (LPNMTOOLBAR) lParam;
          if(pToolBar->hdr.hwndFrom == g_toolbar.hwnd && pToolBar->iItem == IDM_COMPRESSOR) {
            ShowToolbarDropdownMenu();
            return TBDDRET_DEFAULT;
          }
          else {
            return TBDDRET_NODEFAULT;
          }
        }
      }
      return TRUE;
    case WM_COPYDATA:
    {
      PCOPYDATASTRUCT cds = PCOPYDATASTRUCT(lParam);
      switch (cds->dwData) {
        case MAKENSIS_NOTIFY_SCRIPT:
          if (g_sdata.input_script) GlobalFree(g_sdata.input_script);
          g_sdata.input_script = (char *)GlobalAlloc(GPTR, cds->cbData);
          lstrcpy(g_sdata.input_script, (char *)cds->lpData);
          break;
        case MAKENSIS_NOTIFY_WARNING:
          g_sdata.warnings++;
          break;
        case MAKENSIS_NOTIFY_ERROR:
          break;
        case MAKENSIS_NOTIFY_OUTPUT:
          if (g_sdata.output_exe) GlobalFree(g_sdata.output_exe);
          g_sdata.output_exe = (char *)GlobalAlloc(GPTR, cds->cbData);
          lstrcpy(g_sdata.output_exe, (char *)cds->lpData);
          break;
      }
      return TRUE;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDM_BROWSESCR: {
          if (g_sdata.input_script) {
            char str[MAX_PATH],*str2;
            lstrcpy(str,g_sdata.input_script);
            str2=my_strrchr(str,'\\');
            if(str2!=NULL) *str2=0;
            ShellExecute(g_sdata.hwnd,"open",str,NULL,NULL,SW_SHOWNORMAL);
          }
          return TRUE;
        }
        case IDM_ABOUT:
        {
          DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_ABOUT),g_sdata.hwnd,(DLGPROC)AboutProc);
          return TRUE;
        }
        case IDM_NSISHOME:
        {
          ShellExecute(g_sdata.hwnd,"open",NSIS_URL,NULL,NULL,SW_SHOWNORMAL);
          return TRUE;
        }
        case IDM_FORUM:
        {
          ShellExecute(g_sdata.hwnd,"open",NSIS_FOR,NULL,NULL,SW_SHOWNORMAL);
          return TRUE;
        }
        case IDM_NSISUPDATE:
        {
          int rv;
          rv = MessageBox(g_sdata.hwnd,NSISUPDATEPROMPT,"Question",MB_YESNO|MB_ICONQUESTION);
          if(rv == IDYES) {
            char pathf[MAX_PATH],*path;
            GetModuleFileName(NULL,pathf,sizeof(pathf));
            path=my_strrchr(pathf,'\\');
            if(path!=NULL) *path=0;
            lstrcat(pathf,NSIS_UPDATE);
            ShellExecute(g_sdata.hwnd,"open",pathf,NULL,NULL,SW_SHOWNORMAL);
            PostMessage(g_sdata.hwnd,WM_COMMAND, IDM_EXIT, 0);
          }
          return TRUE;
        }
        case IDM_SELECTALL:
        {
          SendDlgItemMessage(g_sdata.hwnd, IDC_LOGWIN, EM_SETSEL, 0, -1);
          return TRUE;
        }
        case IDM_DOCS:
        {
          ShowDocs();
          return TRUE;
        }
        case IDM_LOADSCRIPT:
        {
          if (!g_sdata.thread) {
            OPENFILENAME l={sizeof(l),};
            char buf[MAX_PATH];
            l.hwndOwner = hwndDlg;
            l.lpstrFilter = "NSIS Script (*.nsi)\0*.nsi\0All Files (*.*)\0*.*\0";
            l.lpstrFile = buf;
            l.nMaxFile = MAX_STRING-1;
            l.lpstrTitle = "Load Script";
            l.lpstrDefExt = "log";
            l.lpstrFileTitle = NULL;
            l.lpstrInitialDir = NULL;
            l.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
            lstrcpy(buf,"");
            if (GetOpenFileName(&l)) {
              g_sdata.script = (char *)GlobalAlloc(GPTR,lstrlen(buf)+3);
              wsprintf(g_sdata.script,"\"%s\"",buf);
              PushMRUFile(g_sdata.script);
              ResetObjects();
              CompileNSISScript();
            }
          }
          return TRUE;
        }
        case IDM_MRU_FILE:
        case IDM_MRU_FILE+1:
        case IDM_MRU_FILE+2:
        case IDM_MRU_FILE+3:
        case IDM_MRU_FILE+4:
          LoadMRUFile(LOWORD(wParam)-IDM_MRU_FILE);
          return TRUE;
        case IDM_CLEAR_MRU_LIST:
          ClearMRUList();
          return TRUE;
        case IDM_COMPRESSOR:
        {
          SetCompressor((NCOMPRESSOR)(g_sdata.compressor+1));
          return TRUE;
        }
        case IDM_CLEARLOG:
        {
          if (!g_sdata.thread) {
            ClearLog(g_sdata.hwnd);
          }
          return TRUE;
        }
        case IDM_RECOMPILE:
        {
          CompileNSISScript();
          return TRUE;
        }
        // Added by Darren Owen (DrO) on 1/10/2003
        case IDM_RECOMPILE_TEST:
        case IDC_RECOMPILE_TEST:
        {
          g_sdata.recompile_test = 1;
          CompileNSISScript();
          return TRUE;
        }
        case IDM_DEFINES:
        {
          DialogBox(g_sdata.hInstance,MAKEINTRESOURCE(DLG_DEFINES),g_sdata.hwnd,(DLGPROC)DefinesProc);
          return TRUE;
        }
        case IDM_TEST:
        case IDC_TEST:
        {
          if (g_sdata.output_exe) {
            ShellExecute(g_sdata.hwnd,"open",g_sdata.output_exe,NULL,NULL,SW_SHOWNORMAL);
          }
          return TRUE;
        }
        case IDM_EDITSCRIPT:
        {
          if (g_sdata.input_script) {
            if ((int)ShellExecute(g_sdata.hwnd,"open",g_sdata.input_script,NULL,NULL,SW_SHOWNORMAL)<=32) {
              char path[MAX_PATH];
              if (GetWindowsDirectory(path,sizeof(path))) {
                lstrcat(path,"\\notepad.exe");
                ShellExecute(g_sdata.hwnd,"open",path,g_sdata.input_script,NULL,SW_SHOWNORMAL);
              }
            }
          }
          return TRUE;
        }
        case IDC_CLOSE:
        case IDM_EXIT:
        {
          if (!g_sdata.thread) {
            DestroyWindow(g_sdata.hwnd);
          }
          return TRUE;
        }
        case IDM_COPY:
        {
          CopyToClipboard(g_sdata.hwnd);
          return TRUE;
        }
        case IDM_COPYSELECTED:
        {
          SendDlgItemMessage(g_sdata.hwnd,IDC_LOGWIN, WM_COPY, 0, 0);
          return TRUE;
        }
        case IDM_SAVE:
        {
          OPENFILENAME l={sizeof(l),};
          char buf[MAX_STRING];
          l.hwndOwner = hwndDlg;
          l.lpstrFilter = "Log Files (*.log)\0*.log\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
          l.lpstrFile = buf;
          l.nMaxFile = MAX_STRING-1;
          l.lpstrTitle = "Save Output";
          l.lpstrDefExt = "log";
          l.lpstrInitialDir = NULL;
          l.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_PATHMUSTEXIST;
          lstrcpy(buf,"output");
          if (GetSaveFileName(&l)) {
            HANDLE hFile = CreateFile(buf,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
            if (hFile) {
              int len=SendDlgItemMessage(g_sdata.hwnd,IDC_LOGWIN,WM_GETTEXTLENGTH,0,0);
              char *existing_text=(char*)GlobalAlloc(GPTR,len);
              existing_text[0]=0;
              GetDlgItemText(g_sdata.hwnd, IDC_LOGWIN, existing_text, len);
              DWORD dwWritten = 0;
              WriteFile(hFile,existing_text,len,&dwWritten,0);
              CloseHandle(hFile);
              GlobalFree(existing_text);
            }
          }
          return TRUE;
        }
        case IDM_FIND:
        {
          if (!g_find.uFindReplaceMsg) g_find.uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
          my_memset(&g_find.fr, 0, sizeof(FINDREPLACE));
          g_find.fr.lStructSize = sizeof(FINDREPLACE);
          g_find.fr.hwndOwner = hwndDlg;
          g_find.fr.Flags = FR_NOUPDOWN;
          g_find.fr.lpstrFindWhat = (char *)GlobalAlloc(GPTR, 128);
          if (!g_find.fr.lpstrFindWhat) return TRUE;
          g_find.fr.wFindWhatLen = 128;
          g_find.hwndFind = FindText(&g_find.fr);
          return TRUE;
        }
        case IDM_DEFAULT:
          SetCompressor(COMPRESSOR_DEFAULT);
          return TRUE;
        case IDM_ZLIB:
          SetCompressor(COMPRESSOR_ZLIB);
          return TRUE;
        case IDM_BZIP2:
          SetCompressor(COMPRESSOR_BZIP2);
          return TRUE;
        case IDM_BEST:
          SetCompressor(COMPRESSOR_BEST);
          return TRUE;
      }
    }
  }
  if (g_find.uFindReplaceMsg && msg == g_find.uFindReplaceMsg) {
    LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;
    if (lpfr->Flags & FR_FINDNEXT) {
      WPARAM flags = FR_DOWN;
      if (lpfr->Flags & FR_MATCHCASE) flags |= FR_MATCHCASE;
      if (lpfr->Flags & FR_WHOLEWORD) flags |= FR_WHOLEWORD;
      FINDTEXTEX ft;
      SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_EXGETSEL, 0, (LPARAM)&ft.chrg);
      if (ft.chrg.cpMax == ft.chrg.cpMin) ft.chrg.cpMin = 0;
      else ft.chrg.cpMin = ft.chrg.cpMax;
      ft.chrg.cpMax = SendDlgItemMessage(hwndDlg, IDC_LOGWIN, WM_GETTEXTLENGTH, 0, 0);
      ft.lpstrText = lpfr->lpstrFindWhat;
      ft.chrg.cpMin = SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_FINDTEXTEX, flags, (LPARAM)&ft);
      if (ft.chrg.cpMin != -1) SendDlgItemMessage(hwndDlg, IDC_LOGWIN, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
      else MessageBeep(MB_ICONASTERISK);
    }
    if (lpfr->Flags & FR_DIALOGTERM) g_find.hwndFind = 0;
    return TRUE;
  }
  return 0;
}

DWORD WINAPI MakeNSISProc(LPVOID p) {
  STARTUPINFO si={sizeof(si),};
  SECURITY_ATTRIBUTES sa={sizeof(sa),};
  SECURITY_DESCRIPTOR sd={0,};
  PROCESS_INFORMATION pi={0,};
  HANDLE newstdout=0,read_stdout=0;
  OSVERSIONINFO osv={sizeof(osv)};
  GetVersionEx(&osv);
  if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd,true,NULL,false);
    sa.lpSecurityDescriptor = &sd;
  }
  else sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = true;
  if (!CreatePipe(&read_stdout,&newstdout,&sa,0)) {
    ErrorMessage(g_sdata.hwnd,"There was an error creating the pipe.");
    PostMessage(g_sdata.hwnd,WM_MAKENSIS_PROCESSCOMPLETE,0,0);
    return 1;
  }
  GetStartupInfo(&si);
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdOutput = newstdout;
  si.hStdError = newstdout;
  if (!CreateProcess(NULL,g_sdata.script,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi)) {
    char buf[MAX_STRING];
    wsprintf(buf,"Could not execute:\r\n %s.",g_sdata.script);
    ErrorMessage(g_sdata.hwnd,buf);
    CloseHandle(newstdout);
    CloseHandle(read_stdout);
    PostMessage(g_sdata.hwnd,WM_MAKENSIS_PROCESSCOMPLETE,0,0);
    return 1;
  }
  char szBuf[1024];
  DWORD dwRead = 1;
  DWORD dwExit = !STILL_ACTIVE;
  while (dwExit == STILL_ACTIVE || dwRead) {
    PeekNamedPipe(read_stdout, 0, 0, 0, &dwRead, NULL);
    if (dwRead) {
      ReadFile(read_stdout, szBuf, sizeof(szBuf)-1, &dwRead, NULL);
      szBuf[dwRead] = 0;
      LogMessage(g_sdata.hwnd, szBuf);
    }
    else Sleep(TIMEOUT);
    GetExitCodeProcess(pi.hProcess, &dwExit);
    // Make sure we have no data before killing getting out of the loop
    if (dwExit != STILL_ACTIVE) {
      PeekNamedPipe(read_stdout, 0, 0, 0, &dwRead, NULL);
    }
  }

  g_sdata.retcode = dwExit;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(newstdout);
  CloseHandle(read_stdout);
  PostMessage(g_sdata.hwnd,WM_MAKENSIS_PROCESSCOMPLETE,0,0);
  return 0;
}

BOOL CALLBACK DialogResize(HWND hWnd, LPARAM /* unused */)
{
  RECT r;
  GetWindowRect(hWnd, &r);
  ScreenToClient(g_sdata.hwnd, (LPPOINT)&r);
  ScreenToClient(g_sdata.hwnd, ((LPPOINT)&r)+1);
  if(hWnd != g_toolbar.hwnd) {
    switch (GetDlgCtrlID(hWnd)) {
      case IDC_LOGWIN:
        SetWindowPos(hWnd, 0, r.left, r.top,r.right - r.left + g_resize.dx, r.bottom - r.top + g_resize.dy, SWP_NOZORDER|SWP_NOMOVE);
        break;
      case IDC_TEST:
      case IDC_CLOSE:
        SetWindowPos(hWnd, 0, r.left + g_resize.dx, r.top + g_resize.dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
        break;
      default:
        SetWindowPos(hWnd, 0, r.left, r.top + g_resize.dy, r.right - r.left + g_resize.dx, r.bottom - r.top, SWP_NOZORDER);
        break;
    }
  }
  RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
  return TRUE;
}

BOOL CALLBACK AboutProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      HFONT bfont = CreateFont(13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FIXED_PITCH|FF_DONTCARE, "Tahoma");
      HFONT bfontb = CreateFont(13,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FIXED_PITCH|FF_DONTCARE, "Tahoma");
      HFONT rfont = CreateFont(12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FIXED_PITCH|FF_DONTCARE, "MS Shell Dlg");
      HFONT rfontb = CreateFont(12,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FIXED_PITCH|FF_DONTCARE, "MS Shell Dlg");
      if (bfont&&bfontb) {
        SendDlgItemMessage(hwndDlg, IDC_ABOUTVERSION, WM_SETFONT, (WPARAM)bfontb, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_ABOUTCOPY, WM_SETFONT, (WPARAM)bfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_ABOUTPORTIONS, WM_SETFONT, (WPARAM)bfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_NSISVER, WM_SETFONT, (WPARAM)bfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_OTHERCONTRIB, WM_SETFONT, (WPARAM)bfont, FALSE);
      }
      else if (rfont&&rfontb) {
        SendDlgItemMessage(hwndDlg, IDC_ABOUTVERSION, WM_SETFONT, (WPARAM)rfontb, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_ABOUTCOPY, WM_SETFONT, (WPARAM)rfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_ABOUTPORTIONS, WM_SETFONT, (WPARAM)rfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_NSISVER, WM_SETFONT, (WPARAM)rfont, FALSE);
        SendDlgItemMessage(hwndDlg, IDC_OTHERCONTRIB, WM_SETFONT, (WPARAM)rfont, FALSE);
      }
      SetDlgItemText(hwndDlg,IDC_NSISVER,g_sdata.branding);
      SetDlgItemText(hwndDlg,IDC_ABOUTVERSION,NSISW_VERSION);
      SetDlgItemText(hwndDlg,IDC_ABOUTCOPY,COPYRIGHT);
      SetDlgItemText(hwndDlg,IDC_OTHERCONTRIB,CONTRIB);
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDOK:
          EndDialog(hwndDlg, TRUE);
          break;
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK DefinesProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_INITDIALOG:
    {
      if(g_sdata.defines) {
        int i=0;
        while(g_sdata.defines[i]) {
          SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_ADDSTRING, 0, (LPARAM)g_sdata.defines[i]);
          i++;
        }
      }
      EnableWindow(GetDlgItem(hwndDlg, IDRIGHT), FALSE);
      EnableWindow(GetDlgItem(hwndDlg, IDLEFT), FALSE);
      SetFocus(GetDlgItem(hwndDlg, IDC_SYMBOL));
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam)) {
        case IDOK:
        {
          ResetObjects();
          ResetInputScript();
          ResetDefines();
          int n = SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETCOUNT, 0, 0);
          if(n > 0) {
            g_sdata.defines = (char **)GlobalAlloc(GPTR, (n+1)*sizeof(char *));
            for(int i=0; i<n; i++) {
              int len = SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETTEXTLEN, (WPARAM)i, 0);
              g_sdata.defines[i] = (char *)GlobalAlloc(GPTR, (len+1)*sizeof(char));
              SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETTEXT, (WPARAM)i, (LPARAM)g_sdata.defines[i]);
            }
            g_sdata.defines[n] = NULL;
          }
          EndDialog(hwndDlg, TRUE);
        }
        break;
        case IDCANCEL:
          EndDialog(hwndDlg, TRUE);
          break;
        case IDRIGHT:
        {
          int n = SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXTLENGTH, 0, 0);
          if(n > 0) {
            char *buf = (char *)GlobalAlloc(GPTR, (n+1)*sizeof(char));
            SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXT, n+1, (LPARAM)buf);
            if(my_strstr(buf," ") || my_strstr(buf,"\t")) {
              MessageBox(hwndDlg,DEFINESERROR,"Error",MB_OK|MB_ICONSTOP);
              GlobalFree(buf);
              break;
            }

            n = SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_GETTEXTLENGTH, 0, 0);
            if(n > 0) {
              char *buf2 = (char *)GlobalAlloc(GPTR, (n+1)*sizeof(char));
              SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_GETTEXT, n+1, (LPARAM)buf2);
              char *buf3 = (char *)GlobalAlloc(GPTR, (lstrlen(buf)+lstrlen(buf2)+2)*sizeof(char));
              wsprintf(buf3,"%s=%s",buf,buf2);
              GlobalFree(buf);
              buf = buf3;
              GlobalFree(buf2);
            }
            SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_ADDSTRING, 0, (LPARAM)buf);
            SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_SETTEXT, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_SETTEXT, 0, 0);
            GlobalFree(buf);
          }
        }
        break;
      case IDLEFT:
      {
        int index = SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETCURSEL, 0, 0);
        if(index != LB_ERR) {
          int n = SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETTEXTLEN, (WPARAM)index, 0);
          if(n > 0) {
            char *buf = (char *)GlobalAlloc(GPTR, (n+1)*sizeof(char));
            SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETTEXT, (WPARAM)index, (LPARAM)buf);
            char *p = my_strstr(buf,"=");
            if(p) {
              SendDlgItemMessage(hwndDlg, IDC_VALUE, WM_SETTEXT, 0, (LPARAM)(p+1));
              *p=0;
            }
            SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_SETTEXT, 0, (LPARAM)buf);
            GlobalFree(buf);
            SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_DELETESTRING, (WPARAM)index, (LPARAM)buf);
            EnableWindow(GetDlgItem(hwndDlg, IDLEFT), FALSE);
          }
        }
      }
      break;
      case IDC_SYMBOL:
        if(HIWORD(wParam) == EN_CHANGE)
        {
          int n = SendDlgItemMessage(hwndDlg, IDC_SYMBOL, WM_GETTEXTLENGTH, 0, 0);
          if(n > 0) {
            EnableWindow(GetDlgItem(hwndDlg, IDRIGHT), TRUE);
          }
          else {
            EnableWindow(GetDlgItem(hwndDlg, IDRIGHT), FALSE);
          }
        }
        break;
      case IDC_DEFINES:
        if(HIWORD(wParam) == LBN_SELCHANGE)
        {
          int n = SendDlgItemMessage(hwndDlg, IDC_DEFINES, LB_GETCURSEL, 0, 0);
          if(n != LB_ERR) {
            EnableWindow(GetDlgItem(hwndDlg, IDLEFT), TRUE);
          }
          else {
            EnableWindow(GetDlgItem(hwndDlg, IDLEFT), FALSE);
          }
        }
        break;
      }
    break;
    }
  }
  return FALSE;
}

void SetCompressor(NCOMPRESSOR compressor)
{
  if(g_sdata.compressor != compressor) {
    WORD command;
    char *compressor_name;

    switch(compressor) {
      case COMPRESSOR_ZLIB:
        command = IDM_ZLIB;
        compressor_name = ZLIB_COMPRESSOR_NAME;
        break;
      case COMPRESSOR_BZIP2:
        command = IDM_BZIP2;
        compressor_name = BZIP2_COMPRESSOR_NAME;
        break;
      case COMPRESSOR_BEST:
        command = IDM_BEST;
        compressor_name = ZLIB_COMPRESSOR_NAME;
        break;
      default:
        compressor = COMPRESSOR_DEFAULT;
        command = IDM_DEFAULT;
        compressor_name = "";
    }
    g_sdata.compressor = compressor;
    g_sdata.compressor_name = compressor_name;
    UpdateToolBarCompressorButton();
    CheckMenuItem(g_sdata.menu, IDM_DEFAULT, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(g_sdata.menu, IDM_ZLIB, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(g_sdata.menu, IDM_BZIP2, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(g_sdata.menu, IDM_BEST, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(g_sdata.menu, command, MF_BYCOMMAND | MF_CHECKED);
    ResetObjects();
    ResetInputScript();
  }
}
