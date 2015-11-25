/*	TCP Client/Server Queuing Example	Steve Falkenburg, MacDTS, Apple Computer	3/11/92		this client/server sample uses MacTCP to implement a simple "greeting" server.  the server	opens up several listeners on kGreetingPort (1235).  when a client connects, the data entered	in the greeting dialog is sent to the remote connection, and the connection is closed.		connection management is done through the use of Operating System queues to simplify tracking	and usage.*/#include <Processes.h>#include <MacTCPCommonTypes.h>#include <TCPPB.h>#include "const.h"#include "globals.h"#include "utils.h"#include "queues.h"#include "network.h"void StartListener(TCPiopb *pBlock);void ListenerCompletion(MyQElemPtr iopb);void SendData(TCPiopb *pBlock,StringPtr data);/*	initialize TCP/IP driver and create our queues, putting them on the unused list*/OSErr InitNetwork(void){	OSErr err;	TCPiopb *pBlock;	short i;		err = OpenDriver("\p.ipp",&gTcpDrvrRef);	if (err!=noErr)		return err;		for (i=0; i<kNumConnections; i++) {		pBlock = (TCPiopb *)GetUnusedPBlock();		if (pBlock)			StartListener(pBlock);	}		return noErr;}/*	creates a connection end and assigns it to listen for an incoming connection on our	network port.*/void StartListener(TCPiopb *pBlock){	Ptr rcvBuff;	OSErr err;			rcvBuff = NewPtr(kRcvBuffSize);	if (MemError()!=noErr)		DoError(MemError());		/* create a network stream */		pBlock->csCode = TCPCreate;	pBlock->ioCRefNum = gTcpDrvrRef;	pBlock->csParam.create.rcvBuff = rcvBuff;	pBlock->csParam.create.rcvBuffLen = kRcvBuffSize;	pBlock->csParam.create.notifyProc = nil;	err = PBControl(pBlock,false);	if (err!=noErr)		DoError(err);		/* create a listener */		pBlock->csCode = TCPPassiveOpen;#ifdef __SYSEQU__	((MyQElemPtr)pBlock)->savedA5 = *(long *)CurrentA5;#else		((MyQElemPtr)pBlock)->savedA5 = (long)CurrentA5;#endif	pBlock->ioCompletion = (ProcPtr)ListenerCompletion;	pBlock->csParam.open.ulpTimeoutValue = 0;	pBlock->csParam.open.ulpTimeoutAction = 1;	pBlock->csParam.open.validityFlags = 0xC0;	pBlock->csParam.open.commandTimeoutValue = 0;	pBlock->csParam.open.remoteHost = 0;	pBlock->csParam.open.remotePort = 0;	pBlock->csParam.open.localHost = 0;	pBlock->csParam.open.localPort = kGreetingPort;	pBlock->csParam.open.tosFlags = 0;	pBlock->csParam.open.precedence = 0;	pBlock->csParam.open.dontFrag = 0;	pBlock->csParam.open.timeToLive = 0;	pBlock->csParam.open.security = 0;	pBlock->csParam.open.optionCnt = 0;	PBControl(pBlock,true);	gRunning++; // increment count of running parameter blocks}/*	called to release all network resources in response to a quit*/void CloseNetwork(void){	THz theZone;	short drvrRefNum;	OSErr err;	TCPiopb tcpBlock;	StreamPtr *curStream;	long theStream;	theZone = ApplicZone();		tcpBlock.ioCRefNum = gTcpDrvrRef;	tcpBlock.csCode = TCPGlobalInfo;	err = PBControl((ParmBlkPtr)&tcpBlock,false);	if (err!=noErr)		return;		curStream = *tcpBlock.csParam.globalInfo.tcpCDBTable;	while (*curStream) {			theStream = *curStream;				if (PtrZone((Ptr)theStream)==theZone) {				// only release streams in our heap 			tcpBlock.csCode = TCPStatus;			tcpBlock.tcpStream = theStream;			err = PBControl((ParmBlkPtr)&tcpBlock,false);			// abort connection 						tcpBlock.csCode = TCPAbort;			tcpBlock.tcpStream = theStream;			err = PBControl((ParmBlkPtr)&tcpBlock,false);						// release stream						tcpBlock.csCode = TCPRelease;			tcpBlock.tcpStream = theStream;			err = PBControl((ParmBlkPtr)&tcpBlock,false);		}				curStream++;	}}/*	this completion routine will be called when one of the listeners gets a connection from a	remote machine.  since we're using OS queues dispatched from the main event loop, we just	take the parameter block and put it on the completed queue.  if running seven, we call	WakeUpProcess() to return control to the application*/void ListenerCompletion(MyQElemPtr iopb){	long saveA5;	ProcessSerialNumber currentProc;		saveA5 = SetA5(iopb->savedA5);		StoreCompletedPBlock(iopb);	gRunning--;		if (gRunningSeven) {		WakeUpProcess(&gOurPSN);	}		SetA5(saveA5);}/*	this routine is called from the event dispatcher of the event loop to continue processing	of completed listens received at interrupt time from ListenerCompletion.  We send them	some data, close the connection, and restart the listener, putting it back on MacTCP's queue*/void ProcessConnection(MyQElemPtr iopb){	SendData(&iopb->tcpBlock,gGreetingData);	RecycleFreePBlock(iopb);	iopb = GetUnusedPBlock();	if (iopb)		StartListener(&iopb->tcpBlock);}/*	here, we send some data to the remote host over the opened connection and close the connection	when done.  if we didn't use os queues, this code would be much more complicated, since it	would have to be interrupt safe, and we would have to perform all driver calls async with	linked completion routines (yuck)*/void SendData(TCPiopb *pBlock,StringPtr data){	OSErr err;	wdsEntry wdsPtr[4];	char crlf[] = "\015\012";	wdsPtr[0].length = wdsPtr[2].length = 2;	wdsPtr[0].ptr = wdsPtr[2].ptr = crlf;	wdsPtr[1].length = data[0];	wdsPtr[1].ptr = (void *)&data[1];	wdsPtr[3].length = 0;	wdsPtr[3].ptr = nil;	pBlock->csCode = TCPSend;	pBlock->ioCRefNum = gTcpDrvrRef;	pBlock->csParam.send.ulpTimeoutValue = 0;	pBlock->csParam.send.ulpTimeoutAction = 1;	pBlock->csParam.send.validityFlags = 0xc0;	pBlock->csParam.send.pushFlag = false;	pBlock->csParam.send.urgentFlag = false;	pBlock->csParam.send.wdsPtr = (Ptr)wdsPtr;	err = PBControl(pBlock,false);	if (err!=noErr)		DoError(err);	pBlock->csCode = TCPClose;	pBlock->ioCRefNum = gTcpDrvrRef;	pBlock->csParam.close.validityFlags = 0xC0;	pBlock->csParam.close.ulpTimeoutValue = 20;	pBlock->csParam.close.ulpTimeoutAction = 1;	err = PBControl(pBlock,false);	if (err!=noErr)		DoError(err);		pBlock->csCode = TCPRelease;	pBlock->ioCRefNum = gTcpDrvrRef;	err = PBControl(pBlock,false);	if (err!=noErr)		DoError(err);}