/*-------------------------------------------
  timer.c - Kazubon 1998-1999
---------------------------------------------*/
// Last Modified by Stoic Joker: Sunday, 03/13/2011 @ 11:54:05am
#include "tclock.h"

wchar_t g_szTimersSubKey[] = L"Timers";
//==================================================================
//----------+++--> enumerate & display all times to our context menu:
void UpdateTimerMenu(HMENU hMenu)   //------------------------+++-->
{
	unsigned count = api.GetInt(g_szTimersSubKey, L"NumberOfTimers", 0);
	if(count){
		unsigned idx;
		wchar_t buf[GEN_BUFF+16];
		wchar_t subkey[TNY_BUFF];
		size_t offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
		wcscpy(buf, L"    ");
		EnableMenuItem(hMenu,IDM_TIMEWATCH,MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,IDM_TIMEWATCHRESET,MF_BYCOMMAND|MF_ENABLED);
		InsertMenu(hMenu,IDM_TIMER,MF_BYCOMMAND|MF_SEPARATOR,0,NULL);
		for(idx=0; idx<count; ++idx){
			wchar_t* pos = buf+4;
			wsprintf(subkey+offset, FMT("%d"), idx+1);
			pos += api.GetStr(subkey, L"Name", pos, GEN_BUFF, L"");
			wsprintf(pos, FMT("	(%i"), idx+1);
			InsertMenu(hMenu,IDM_TIMER,MF_BYCOMMAND|MF_STRING,IDM_I_TIMER+idx,buf);
			if(api.GetInt(subkey,L"Active",0))
				CheckMenuItem(hMenu,IDM_I_TIMER+idx,MF_BYCOMMAND|MF_CHECKED);
		}
		InsertMenu(hMenu,IDM_TIMER,MF_BYCOMMAND|MF_SEPARATOR,0,NULL);
	}
}

// Structure for Timer Setting
typedef struct{
	int second;
	int minute;
	int hour;
	int day;
	wchar_t name[GEN_BUFF];
	wchar_t fname[MAX_BUFF];
	char bActive;
	char bRepeat;
	char bBlink;
} timeropt_t;

// Structure for Active Timers
typedef struct{
	int id;
	DWORD seconds;		// Second  = 1 Second
	DWORD tickonstart;	// Minute = 60 Seconds
	wchar_t name[GEN_BUFF];// Hour = 3600 Seconds
	char bHomeless;		// Day = 86400 Seconds
} timer_t;

static int m_timers = 0;
static timer_t* m_timer = NULL; // Array of Currently Active Timers

//========================================================================
//---------------------//---------------------------+++--> Clear All Timer:
void EndAllTimers()   //--------------------------------------------+++-->
{
	m_timers=0;
	free(m_timer),m_timer=NULL;
}
//=================================================================================
//----------------------------------------------+++--> Free Memory to Clear a Timer:
void StopTimer(int id)   //--------------------------------------------------+++-->
{
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	int idx;
	
	for(idx=0; idx<m_timers; ++idx) {
		if(id == m_timer[idx].id){
			timer_t* told=m_timer;
			offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
			wsprintf(subkey+offset, FMT("%d"), m_timer[idx].id+1);
			api.SetInt(subkey, L"Active", 0);
			if(--m_timers){
				int i;
				timer_t* tnew = (timer_t*)malloc(sizeof(timer_t)*m_timers);
				if(!tnew){
					++m_timers; return;
				}
				for(i=0; i<idx; ++i){
					tnew[i] = told[i];
				}
				for(i=idx; i<m_timers; ++i){
					tnew[i] = told[i+1];
				}
				m_timer = tnew;
			}else
				m_timer = NULL;
			Sleep(0);
			free(told);
			return;
		}
	}
}
//=================================================================================
//----------------------------------------------+++--> Free Memory to Clear a Timer:
void StartTimer(int id)   //-------------------------------------------------+++-->
{
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	int idx;
	timer_t* told=m_timer;
	timer_t* tnew;
	for(idx=0; idx<m_timers; ++idx) {
		if(id == m_timer[idx].id)
			return;
	}
	tnew = (timer_t*)malloc(sizeof(timer_t)*(m_timers+1));
	if(!tnew)
		return;
	for(idx=0; idx<m_timers; ++idx){
		tnew[idx] = told[idx];
	}
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	wsprintf(subkey+offset, FMT("%d"), id+1);
	api.GetStr(subkey, L"Name", tnew[m_timers].name, _countof(tnew[m_timers].name), L"");
	if(!*tnew[m_timers].name){
		free(tnew);
		return;
	}
	tnew[m_timers].id = id;
	tnew[m_timers].seconds = api.GetInt(subkey, L"Seconds",0);
	tnew[m_timers].seconds += api.GetInt(subkey, L"Minutes",0) * 60;
	tnew[m_timers].seconds += api.GetInt(subkey, L"Hours",0) * 3600;
	tnew[m_timers].seconds += api.GetInt(subkey, L"Days",0) * 86400;
	tnew[m_timers].bHomeless = 1;
	tnew[m_timers].tickonstart = GetTickCount()/1000;
	m_timer = tnew;
	++m_timers;
	free(told);
	api.SetInt(subkey, L"Active", 1);
}
//=================================================================
//----------------------------------------------+++--> Toggle timer:
void ToggleTimer(int id)   //--------------------------------+++-->
{
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	wsprintf(subkey+offset, FMT("%d"), id+1);
	if(api.GetInt(subkey,L"Active",0)){
		StopTimer(id);
	}else{
		StartTimer(id);
	}
}

static void OnOK(HWND hwnd);
static void OnDel(HWND hwnd);
static void OnInit(HWND hwnd);
static void OnDestroy(HWND hwnd);
static void OnStopTimer(HWND hwnd);
static void OnTimerName(HWND hwnd);
static void Ring(HWND hwnd, int id);
static void OnTest(HWND hwnd, WORD id);
static void OnSanshoAlarm(HWND hwnd, WORD id);
static void UpdateNextCtrl(HWND hWnd, int iSpin, int iEdit, char bGoUp);

INT_PTR CALLBACK Window_Timer(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
//=========================================================================================*
// ------------------------------------------------------------- Open Add/Edit Timers Dialog
//===========================================================================================*
void DialogTimer()
{
	CreateDialogParamOnce(&g_hDlgTimer, 0, MAKEINTRESOURCE(IDD_TIMER), NULL, Window_Timer, 0);
}
//==============================================================================*
// ---------------------------------- Dialog Procedure for Add/Edit Timers Dialog
//================================================================================*
INT_PTR CALLBACK Window_Timer(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		OnInit(hwnd);
		return TRUE;
	case WM_DESTROY:
		OnDestroy(hwnd);
		break;
		
	case WM_COMMAND: {
			WORD id = LOWORD(wParam);
			WORD code = HIWORD(wParam);
			switch(id) {
			case IDC_TIMERDEL:
				OnDel(hwnd);
				break;
				
			case IDOK:
				OnOK(hwnd);
				/* fall through */
			case IDCANCEL:
				DestroyWindow(hwnd);
				break;
				
			case IDC_TIMERNAME:
				if(code==CBN_EDITCHANGE) OnTimerName(hwnd);
				else if(code==CBN_SELCHANGE) PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, CBN_EDITCHANGE), lParam);
				break;
				
			case IDCB_STOPTIMER:
				OnStopTimer(hwnd);
				break;
				
			case IDC_TIMERSANSHO:
				OnSanshoAlarm(hwnd, id);
				break;
				
			case IDC_TIMERTEST:
				OnTest(hwnd, id);
				break;
			}
			return TRUE;
		}
		//--------------------------------------------------------------------------+++-->
	case WM_NOTIFY: { //========================================== BEGIN WM_NOTIFY:
//----------------------------------------------------------------------------+++-->
			if(((NMHDR*)lParam)->code == UDN_DELTAPOS) {
				NMUPDOWN* lpnmud;
				int i;
				
				lpnmud = (NMUPDOWN*)lParam;
				if(lpnmud->iDelta > 0) { // User Selected the Up Arrow
					switch(LOWORD(wParam)) { //--+++--> on One of the Timer Controls.
					case IDC_TIMERSECSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERSECOND, NULL, TRUE);
						if(i == 59)
							UpdateNextCtrl(hwnd, IDC_TIMERMINSPIN, IDC_TIMERMINUTE, TRUE);
						break;
						
					case IDC_TIMERMINSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERMINUTE, NULL, TRUE);
						if(lpnmud->iDelta == 4) {
							if(i < 59)
								SetDlgItemInt(hwnd, IDC_TIMERMINUTE, i+1, TRUE);
						}
						if(i == 59)
							UpdateNextCtrl(hwnd, IDC_TIMERHORSPIN, IDC_TIMERHOUR, TRUE);
						break;
						
					case IDC_TIMERHORSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERHOUR, NULL, TRUE);
						if(lpnmud->iDelta == 4) {
							if(i < 23)
								SetDlgItemInt(hwnd, IDC_TIMERHOUR, i+1, TRUE);
						}
						if(i == 23)
							UpdateNextCtrl(hwnd, IDC_TIMERDAYSPIN, IDC_TIMERDAYS, TRUE);
						break;
						
					case IDC_TIMERDAYSPIN:
						if(lpnmud->iDelta == 4) {
							i = GetDlgItemInt(hwnd, IDC_TIMERDAYS, NULL, TRUE);
							if(i < 7)
								SetDlgItemInt(hwnd, IDC_TIMERDAYS, i+1, TRUE);
						} break;
					}
				} else { //--+++--> User Selected the Down Arrow
					switch(LOWORD(wParam)) { // on One of the Timer Controls.
					case IDC_TIMERSECSPIN:
						if(lpnmud->iDelta == -4) {
							i = GetDlgItemInt(hwnd, IDC_TIMERSECOND, NULL, TRUE);
							if(i > 0)
								SetDlgItemInt(hwnd, IDC_TIMERSECOND, i -1, TRUE);
						} break;
						
					case IDC_TIMERMINSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERMINUTE, NULL, TRUE);
						if(lpnmud->iDelta == -4) {
							if(i > 0)
								SetDlgItemInt(hwnd, IDC_TIMERMINUTE, i -1, TRUE);
						}
						if(i == 0)
							UpdateNextCtrl(hwnd, IDC_TIMERSECSPIN, IDC_TIMERSECOND, FALSE);
						break;
						
					case IDC_TIMERHORSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERHOUR, NULL, TRUE);
						if(lpnmud->iDelta == -4) {
							if(i > 0)
								SetDlgItemInt(hwnd, IDC_TIMERHOUR, i -1, TRUE);
						}
						if(i == 0)
							UpdateNextCtrl(hwnd, IDC_TIMERMINSPIN, IDC_TIMERMINUTE, FALSE);
						break;
						
					case IDC_TIMERDAYSPIN:
						i = GetDlgItemInt(hwnd, IDC_TIMERDAYS, NULL, TRUE);
						if(i == 0)
							UpdateNextCtrl(hwnd, IDC_TIMERHORSPIN, IDC_TIMERHOUR, FALSE);
						break;
					}
				}
			}
//----------------------------------------------------------------------------+++-->
			return TRUE; //=============================================== END WM_NOTIFY:
		} //----------------------------------------------------------------------+++-->
		
	case MM_MCINOTIFY:
	case MM_WOM_DONE:
		StopFile();
		SendDlgItemMessage(hwnd, IDC_TIMERTEST, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hIconPlay);
		return TRUE;
	}
	return FALSE;
}
//================================================================================================
//------------------------//------------------------+++--> free memories associated with combo box:
void OnDestroy(HWND hwnd)   //--------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hwnd, IDC_TIMERNAME);
	int idx;
	int count = ComboBox_GetCount(timer_cb);
	StopFile();
	for(idx=0; idx<count; ++idx) {
		free((void*)ComboBox_GetItemData(timer_cb,idx));
	}
	g_hDlgTimer = NULL;
}
//================================================================================================
//------------------------//----------------------------------+++--> Initialize the "Timer" Dialog:
void OnInit(HWND hwnd)   //-----------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hwnd, IDC_TIMERNAME);
	HWND file_cb = GetDlgItem(hwnd, IDC_TIMERFILE);
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	int idx, count;
	timeropt_t* pts;
	
	SendMessage(hwnd, WM_SETICON, ICON_SMALL,(LPARAM)g_hIconTClock);
	SendMessage(hwnd, WM_SETICON, ICON_BIG,(LPARAM)g_hIconTClock);
	// init dialog items
	SendDlgItemMessage(hwnd, IDC_TIMERSECSPIN, UDM_SETRANGE32, 0,59); // 60 Seconds Max
	SendDlgItemMessage(hwnd, IDC_TIMERMINSPIN, UDM_SETRANGE32, 0,59); // 60 Minutes Max
	SendDlgItemMessage(hwnd, IDC_TIMERHORSPIN, UDM_SETRANGE32, 0,23); // 24 Hours Max
	SendDlgItemMessage(hwnd, IDC_TIMERDAYSPIN, UDM_SETRANGE32, 0,7); //  7 Days Max
	/// add default sound files to file dropdown
	ComboBoxArray_AddSoundFiles(&file_cb, 1);
	// add timer to combobox
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	count = api.GetInt(g_szTimersSubKey, L"NumberOfTimers", 0);
	for(idx=0; idx<count; ++idx) {
		pts = (timeropt_t*)malloc(sizeof(timeropt_t));
		wsprintf(subkey+offset, FMT("%d"), idx+1);
		pts->second = api.GetInt(subkey, L"Seconds",  0);
		pts->minute = api.GetInt(subkey, L"Minutes", 10);
		pts->hour   = api.GetInt(subkey, L"Hours",    0);
		pts->day    = api.GetInt(subkey, L"Days",     0);
		api.GetStr(subkey, L"Name", pts->name, _countof(pts->name), L"");
		api.GetStr(subkey, L"File", pts->fname, _countof(pts->fname), L"");
		pts->bBlink = (char)api.GetInt(subkey, L"Blink", 0);
		pts->bRepeat = (char)api.GetInt(subkey, L"Repeat", 0);
		pts->bActive = (char)api.GetInt(subkey, L"Active", 0);
		ComboBox_AddString(timer_cb, pts->name);
		ComboBox_SetItemData(timer_cb, idx, pts);
	}
	// add "new timer" item
	pts = (timeropt_t*)calloc(1, sizeof(timeropt_t));
	wcscpy(pts->name, L"<Add New...>");
	ComboBox_AddString(timer_cb, pts->name);
	ComboBox_SetItemData(timer_cb, count, pts);
	ComboBox_SetCurSel(timer_cb, 0);
	OnTimerName(hwnd);
	SendDlgItemMessage(hwnd, IDC_TIMERTEST, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hIconPlay);
	SendDlgItemMessage(hwnd, IDC_TIMERDEL, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hIconDel);
	
	api.PositionWindow(hwnd,21);
}
//================================================================================================
//--{ START TIMER }-----//-----------------+++--> Called When "OK" Button is Clicked (Start Timer):
void OnOK(HWND hwnd)   //-------------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hwnd, IDC_TIMERNAME);
	int idx, count, seconds, minutes, hours, days;
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	wchar_t name[GEN_BUFF];
	wchar_t fname[MAX_PATH];
	
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	ComboBox_GetText(timer_cb, name, _countof(name));
	
	count = ComboBox_GetCount(timer_cb);
	count -= 1; // Skip the Last One Because It's the New Timer Dummy Item
	
	for(idx=0; idx<count; ++idx) {
		timeropt_t* pts;
		pts = (timeropt_t*)ComboBox_GetItemData(timer_cb, idx);
		if(!wcscmp(pts->name, name)) {
			pts->bActive = TRUE;
			break;
		}
	}
	wsprintf(subkey+offset, FMT("%d"), idx+1);
	api.SetStr(subkey, L"Name", name);
	seconds = GetDlgItemInt(hwnd, IDC_TIMERSECOND, 0, 0);
	minutes = GetDlgItemInt(hwnd, IDC_TIMERMINUTE, 0, 0);
	hours   = GetDlgItemInt(hwnd, IDC_TIMERHOUR,   0, 0);
	days    = GetDlgItemInt(hwnd, IDC_TIMERDAYS,   0, 0);
	if(seconds>59) for(; seconds>59; seconds-=60,++minutes);
	if(minutes>59) for(; minutes>59; minutes-=60,++hours);
	if(hours>23) for(; hours>23; hours-=24,++days);
	if(days > 42)
		days = 7;
	api.SetInt(subkey, L"Seconds", seconds);
	api.SetInt(subkey, L"Minutes", minutes);
	api.SetInt(subkey, L"Hours",   hours);
	api.SetInt(subkey, L"Days",    days);
	
	GetDlgItemText(hwnd, IDC_TIMERFILE, fname, _countof(fname));
	api.SetStr(subkey, L"File", fname);
	
	api.SetInt(subkey, L"Repeat", IsDlgButtonChecked(hwnd, IDC_TIMERREPEAT));
	api.SetInt(subkey, L"Blink",  IsDlgButtonChecked(hwnd, IDC_TIMERBLINK));
	api.SetInt(subkey, L"Active",  TRUE);
	if(idx == count)
		api.SetInt(g_szTimersSubKey, L"NumberOfTimers", idx+1);
	
	StartTimer(idx);
}
//================================================================================================
//-----------------------------//-----+++--> Load the Data Set For Timer X When Its Name is Called:
void OnTimerName(HWND hwnd)   //------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hwnd, IDC_TIMERNAME);
	wchar_t name[TNY_BUFF];
	int idx, count;
	
	ComboBox_GetText(timer_cb, name, _countof(name));
	count = ComboBox_GetCount(timer_cb);
	for(idx=0; idx<count; ++idx){
		timeropt_t* pts;
		pts = (timeropt_t*)ComboBox_GetItemData(timer_cb, idx);
		if(!wcscmp(name, pts->name)){
			SetDlgItemInt(hwnd, IDC_TIMERSECOND,	pts->second, 0);
			SetDlgItemInt(hwnd, IDC_TIMERMINUTE,	pts->minute, 0);
			SetDlgItemInt(hwnd, IDC_TIMERHOUR,		pts->hour,   0);
			SetDlgItemInt(hwnd, IDC_TIMERDAYS,		pts->day,    0);
			ComboBox_AddStringOnce(GetDlgItem(hwnd,IDC_TIMERFILE), pts->fname, 1);
			CheckDlgButton(hwnd, IDC_TIMERREPEAT,	pts->bRepeat);
			CheckDlgButton(hwnd, IDC_TIMERBLINK,	pts->bBlink);
			if(pts->bActive){
				EnableDlgItem(hwnd, IDCB_STOPTIMER, TRUE);
				EnableDlgItem(hwnd, IDOK, FALSE);
			}else{
				EnableDlgItem(hwnd, IDCB_STOPTIMER, FALSE);
				EnableDlgItem(hwnd, IDOK, TRUE);
			}
			break;
		}
	}
	if(idx<count-1){
		SetDlgItemText(hwnd, IDOK, L"Start");
	}else{
		SetDlgItemText(hwnd, IDOK, L"Create");
	}
	EnableDlgItem(hwnd, IDC_TIMERDEL, idx<count-1);
}
/*------------------------------------------------
  browse sound file
--------------------------------------------------*/
void OnSanshoAlarm(HWND hwnd, WORD id)
{
	wchar_t deffile[MAX_PATH], fname[MAX_PATH];
	
	GetDlgItemText(hwnd, id - 1, deffile, MAX_PATH);
	if(!BrowseSoundFile(hwnd, deffile, fname)) // soundselect.c
		return;
	SetDlgItemText(hwnd, id - 1, fname);
	PostMessage(hwnd, WM_NEXTDLGCTL, 1, FALSE);
}

//================================================================================================
//-----------------------//-----------------------------+++--> Delete One of the Configured Timers:
void OnDel(HWND hwnd)   //------------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hwnd, IDC_TIMERNAME);
	wchar_t name[TNY_BUFF];
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	int idx, idx2, count;
	
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	ComboBox_GetText(timer_cb, name, _countof(name));
	count = ComboBox_GetCount(timer_cb) -1;
	for(idx=0; idx<count; ++idx) {
		timeropt_t* pts = (timeropt_t*)ComboBox_GetItemData(timer_cb, idx);
		if(!wcscmp(name,pts->name)){
			break;
		}
	}
	if(idx==count) return;
	StopTimer(idx);
	
	for(idx2=idx+1; idx2<count; ++idx2) {
		timeropt_t* pts;
		pts = (timeropt_t*)ComboBox_GetItemData(timer_cb, idx2);
		wsprintf(subkey+offset, FMT("%d"), idx2); // we're 1 behind, as needed
		api.SetStr(subkey, L"Name",		pts->name);
		api.SetInt(subkey, L"Seconds",	pts->second);
		api.SetInt(subkey, L"Minutes",	pts->minute);
		api.SetInt(subkey, L"Hours",	pts->hour);
		api.SetInt(subkey, L"Days",	pts->day);
		api.SetStr(subkey, L"File",		pts->fname);
		api.SetInt(subkey, L"Repeat",	pts->bRepeat);
		api.SetInt(subkey, L"Blink",	pts->bBlink);
		api.SetInt(subkey, L"Active",	pts->bActive);
	}
	wsprintf(subkey+offset, FMT("%d"), count);
	api.DelKey(subkey);
	api.SetInt(g_szTimersSubKey, L"NumberOfTimers", --count);
	free((void*)ComboBox_GetItemData(timer_cb,idx));
	ComboBox_DeleteString(timer_cb, idx);
	
	ComboBox_SetCurSel(timer_cb, (idx>0)?(idx-1):idx);
	OnTimerName(hwnd);
	PostMessage(hwnd, WM_NEXTDLGCTL, 1, FALSE);
}
//================================================================================================
//---------------------------------//--------------------+++--> Test -> Play/Stop Alarm Sound File:
void OnTest(HWND hwnd, WORD id)   //--------------------------------------------------------+++-->
{
	wchar_t fname[MAX_PATH];
	
	GetDlgItemText(hwnd, id - 2, fname, _countof(fname));
	if(!fname[0])
		return;
	
	if((HICON)SendDlgItemMessage(hwnd, id, BM_GETIMAGE, IMAGE_ICON, 0) == g_hIconPlay) {
		if(PlayFile(hwnd, fname, 0)) {
			SendDlgItemMessage(hwnd, id, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_hIconStop);
			InvalidateRect(GetDlgItem(hwnd, id), NULL, FALSE);
		}
	} else StopFile();
}
//================================================================================================
//------+++--> Called When Main Window Receives WM_TIMER - Sound the Alarm if Clock has Run Out...:
void OnTimerTimer(HWND hwnd)   //-------------------------------------------+++-->
{
	DWORD tick;
	int idx;
	if(!m_timers) return;
	
	tick=GetTickCount()/1000;
	for(idx=0; idx<m_timers; ) {
		DWORD elapsed = tick-m_timer[idx].tickonstart;
		if(elapsed >= m_timer[idx].seconds){
			int id=m_timer[idx].id;
			StopTimer(id);
			Ring(hwnd,id);
		}else
			++idx;
	}
}
//================================================================================================
//------------------------------//---------------------------+++--> Sound Alarm or Open Timer File:
void Ring(HWND hwnd, int id)   //-----------------------------------------------------------+++-->
{
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	wchar_t fname[MAX_BUFF];
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	wsprintf(subkey+offset, FMT("%d"), id+1);
	api.GetStr(subkey, L"File", fname, _countof(fname), L"");
	PlayFile(hwnd, fname, api.GetInt(subkey, L"Repeat", 0)?-1:0);
	if(api.GetInt(subkey, L"Blink", 0))
		PostMessage(g_hwndClock, CLOCKM_BLINK, 0, 0);
}
//================================================================================================
//---------+++--> Get Active Timer Name(s) to Populate Menu -or- Mark Selected Timer as "Homeless":
int GetTimerInfo(wchar_t* dst, int num)   //-----------------------------------+++-->
{
	if(num < m_timers) {
		wchar_t* out=dst;
		DWORD tick = GetTickCount()/1000;
		int iTCount = tick - m_timer[num].tickonstart;
		int days,hours,minutes;
		iTCount = m_timer[num].seconds - iTCount;
		if(iTCount <= 0) {
			return wsprintf(dst, FMT(" <- Time Expired!"));
		}
		days = iTCount/86400; iTCount%=86400;
		hours = iTCount/3600; iTCount%=3600;
		minutes = iTCount/60; iTCount%=60;
		if(days)
			out += wsprintf(out, FMT("%d day%s "), days, (days==1 ? L"" : L"s"));
		out += wsprintf(out, FMT("%02d:%02d:%02d"), hours, minutes, iTCount);
		return (int)(out-dst);
	}
	*dst = '\0';
	return 0;
}
//================================================================================================
//-------------------------------------+++--> Spoof Control Message to Force Update of Nexe Window:
void UpdateNextCtrl(HWND hWnd, int iSpin, int iEdit, char bGoUp)   //-----------------------+++-->
{
	NMUPDOWN nmud;
	
	nmud.hdr.hwndFrom = GetDlgItem(hWnd, iSpin);
	nmud.hdr.idFrom = iSpin;
	nmud.hdr.code = UDN_DELTAPOS;
	if(bGoUp)nmud.iDelta = 4; // Fake Message Forces Update of Next Control!
	else nmud.iDelta = -4;   // Fake Message Forces Update of Next Control!
	nmud.iPos = GetDlgItemInt(hWnd, iEdit, NULL, 1);
	
	SendMessage(hWnd, WM_NOTIFY, iSpin, (LPARAM)&nmud);
}
//================================================================================================
//-----------------------------//-------------------+++--> Stop & Cancel a Currently Running Timer:
void OnStopTimer(HWND hWnd)   //------------------------------------------------------------+++-->
{
	HWND timer_cb = GetDlgItem(hWnd, IDC_TIMERNAME);
	wchar_t name[GEN_BUFF];
	int idx;
	
	ComboBox_GetText(timer_cb, name, _countof(name));
	
	for(idx=0; idx<m_timers; ++idx) {
		if(!wcscmp(name, m_timer[idx].name)) {
			int id=m_timer[idx].id;
			timeropt_t* pts=(timeropt_t*)ComboBox_GetItemData(timer_cb, id);;
			
			StopTimer(id);
			pts->bActive = 0;
			
			EnableDlgItem(hWnd, IDOK, 1);
			EnableDlgItemSafeFocus(hWnd, IDCB_STOPTIMER, 0, IDOK);
			break;
		}
	}
}
//================================================================================================
//-----------------------------+++--> When T-Clock Starts, Make Sure ALL Timer Are Set as INActive:
void CancelAllTimersOnStartUp()   //--------------------------------------------------------+++-->
{
	wchar_t subkey[TNY_BUFF];
	size_t offset;
	int idx, count;
	
	offset = wsprintf(subkey, FMT("%s\\Timer"), g_szTimersSubKey);
	count = api.GetInt(g_szTimersSubKey, L"NumberOfTimers", 0);
	for(idx=0; idx<count; ) {
		wsprintf(subkey+offset, FMT("%d"), ++idx);
		api.SetInt(subkey, L"Active", 0);
	}
}
//================================================================================================
// -------------------------------------------//+++--> Initialize Timer View/Watch Dialog Controls:
void OnInitTimeView(HWND hDlg)   //---------------------------------------------+++-->
{
	LVCOLUMN lvCol;
	HWND hList=GetDlgItem(hDlg,IDC_LIST);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL,(LPARAM)g_hIconTClock);
	SendMessage(hDlg, WM_SETICON, ICON_BIG,(LPARAM)g_hIconTClock);
	
	ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
	SetXPWindowTheme(hList,L"Explorer",NULL);
	
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.cx = 125;		 // Column Width
	lvCol.iSubItem = 0;      // Column Number
	lvCol.fmt = LVCFMT_CENTER; // Column Alignment
	lvCol.pszText = TEXT("Timer"); // Column Header Text
	ListView_InsertColumn(hList, 0, &lvCol);
	
	lvCol.cx = 150;
	lvCol.iSubItem = 1;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = TEXT("Remaining");
	ListView_InsertColumn(hList, 1, &lvCol);
}
//================================================================================================
//-------------------------------------+++--> Gather Status Info About the Timers User is Watching:
BOOL OnWatchTimer(HWND hDlg)   //-----------------------------------------------+++-->
{
	HWND hList=GetDlgItem(hDlg,IDC_LIST);
	wchar_t szStatus[MIN_BUFF];
	BOOL bNeeded = FALSE;
	LVFINDINFO lvFind;
	LVITEM lvItem;
	int id;
	for(id=m_timers; id--; ) {
		if(m_timer[id].bHomeless) {
			int iF;
			GetTimerInfo(szStatus,id);
			
			lvFind.flags = LVFI_STRING;
			lvFind.psz = m_timer[id].name;
			if((iF = ListView_FindItem(hList, -1, &lvFind)) != -1) {
				ListView_SetItemText(hList, iF, 1, szStatus); // IF Timer Pre-Exists,
				bNeeded = TRUE; //------------+++--> Update the Existing Timer Entry.
			} else {
				//---------------------+++--> ELSE Add the New Timer Entry to Watch List.
				lvItem.mask = LVIF_TEXT;
				lvItem.iSubItem = 0;
				lvItem.iItem = 0;
				
				lvItem.pszText = m_timer[id].name;
				ListView_InsertItem(hList, &lvItem);
				
				lvItem.iSubItem = 1;
				lvItem.pszText = szStatus;
				ListView_SetItem(hList, &lvItem);
				bNeeded = TRUE;
			}
		}
	}
	return bNeeded;
}
//================================================================================================
//-----------------------------+++--> Remove Timer X From Timer Watch List (Set Homeless to FALSE):
void RemoveFromWatch(HWND hWnd, HWND hList, wchar_t* szTimer, int iLx)
{
	const wchar_t szMessage[] = L"Yes will cancel the timer & remove it from the Watch List\n"
							L"No will remove timer from Watch List only (timer continues)\n"
							L"Cancel will assume you hit delete accidentally (and do nothing)";
	wchar_t szCaption[GEN_BUFF];
	int idx;
	int id = -1;
							
	for(idx=0; idx<m_timers; ++idx) {
		if(!wcscmp(szTimer, m_timer[idx].name)) {
			id = m_timer[idx].id;
			break;
		}
	}
	
	if(id==-1) { //----+++--> IF the Timer Has Expired...
		ListView_DeleteItem(hList, iLx); // Just Delete it.
		return;
	}
	
	wsprintf(szCaption, FMT("Cancel Timer (%s) Also?"), szTimer);
	
	switch(MessageBox(hWnd, szMessage, szCaption, MB_YESNOCANCEL|MB_ICONQUESTION)) {
	case IDYES:
		ListView_DeleteItem(hList, iLx);
		StopTimer(id); // Does Not Reset Active Flag!
		break;
	case IDNO:
		for(idx=0; idx<m_timers; ++idx) {
			if(!wcscmp(szTimer, m_timer[idx].name)) {
				m_timer[idx].bHomeless = 0;
				ListView_DeleteItem(hList, iLx);
				break;
			}
		}
		break;
	}
}
//===============================================================================================
// ----------------------+++--> Message Processor for the Selected Running Timers Watching Dialog:
INT_PTR CALLBACK Window_TimerView(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)   //---+++-->
{
	switch(msg) {
	case WM_INITDIALOG:
		OnInitTimeView(hwnd);
		SetTimer(hwnd, 3, 285, NULL); // Timer Refresh Times Above 400ms Make
		api.PositionWindow(hwnd,21); //-----+++--> Timer Watch Dialog Appear Sluggish.
		return TRUE; //-------------------------------+++--> END of Case WM_INITDOALOG
//================//================================================================
	case WM_TIMER:
		if(!OnWatchTimer(hwnd)) { // When the Last Monitored Timer
			KillTimer(hwnd, 3);			 // Expires, Close the Now UnNeeded
			g_hDlgTimerWatch = NULL;
			DestroyWindow(hwnd);		 // Timer Watch/View Dialog Window.
		} return TRUE; //--------------------------------+++--> END of Case WM_TIMER
//====================//============================================================
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
			KillTimer(hwnd, 3);
			g_hDlgTimerWatch = NULL;
			DestroyWindow(hwnd);
			return TRUE;
		} return FALSE;//------------------------------+++--> END of Case WM_COMMAND
//===//=============================================================================
	case WM_NOTIFY:
		//--------------------------------------------------------------------+++-->
		if(((NMHDR*)lParam)->code == LVN_KEYDOWN) { //-+> Capture Key Strokes Here.
			LPNMLVKEYDOWN nmkey = (NMLVKEYDOWN*)lParam;
			HWND hList=GetDlgItem(hwnd,IDC_LIST);
			switch(nmkey->wVKey) {
			case VK_DELETE:{
				int i;
				if((i = ListView_GetNextItem(hList,-1,LVNI_SELECTED)) != -1) {
					wchar_t szTimer[GEN_BUFF];
					ListView_GetItemText(hList, i, 0, szTimer, _countof(szTimer));
					RemoveFromWatch(hwnd, hList, szTimer, i);
				}
				return TRUE;}// Delete Key Handled
			}
		} break; //-------------------------------------+++--> END of Case WM_NOTIFY
//===//=============================================================================
	}
	return FALSE;
}
//================================================================================================
// ------------------//---------------------------------------------+++--> Open Timer Watch Dialog:
void WatchTimer(int reset)   //----------------------------------------------------------------------+++-->
{
	if(reset){
		int idx; for(idx=0; idx<m_timers; ++idx) {
			m_timer[idx].bHomeless=1;
		}
	}
	CreateDialogParamOnce(&g_hDlgTimerWatch, 0, MAKEINTRESOURCE(IDD_TIMERVIEW), NULL, Window_TimerView, 0);
}
