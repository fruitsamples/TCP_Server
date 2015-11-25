/*	TCP Client/Server Queuing Example	Steve Falkenburg, MacDTS, Apple Computer	3/11/92		this client/server sample uses MacTCP to implement a simple "greeting" server.  the server	opens up several listeners on kGreetingPort (1235).  when a client connects, the data entered	in the greeting dialog is sent to the remote connection, and the connection is closed.		connection management is done through the use of Operating System queues to simplify tracking	and usage.*/#include "const.h"#include "globals.h"#include "utils.h"#include "queues.h"#include "network.h"#include "events.h"#include "interface.h"#include "main.h"/* main entry point */void main(void){	InitMac();	InitQueues();	InitInterface();	if (InitNetwork()!=noErr)		ExitToShell();			MainLoop();		CloseNetwork();	ExitToShell();}/*	initialize macintosh managers and some globals */void InitMac(void){	SysEnvRec envRec;		InitGraf(&qd.thePort);	InitFonts();	InitWindows();	InitMenus();	TEInit();	InitDialogs(nil);	InitCursor();	FlushEvents(everyEvent,0);		if (SysEnvirons(1,&envRec)!=noErr)		gRunningSeven = false;	else		gRunningSeven = (envRec.systemVersion >= 0x700);		if (gRunningSeven)		GetCurrentProcess(&gOurPSN);}/*	main event loop.  note that we use a *very* large sleeptime if we're running under System 7*/#define	GetSleepTime	(gRunningSeven ? 100:100000)void MainLoop(void){	EventRecord ev;		while (!gDone) {		if (WaitNextEvent(everyEvent,&ev,GetSleepTime,nil)) {			HandleEvent(&ev);		}		else HandleIdleTime(&ev);		UpdateNumberList();	}}