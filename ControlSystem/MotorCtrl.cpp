// MotorCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "ControlSystem.h"
#include "MotorCtrl.h"


// CMotorCtrl

IMPLEMENT_DYNCREATE(CMotorCtrl, CWinThread)

CMotorCtrl::CMotorCtrl()
{
}

CMotorCtrl::~CMotorCtrl()
{
}

BOOL CMotorCtrl::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}

int CMotorCtrl::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

//��ȡ���״̬
void CMotorCtrl::ReadMotorStatus(WPARAM wParam,LPARAM lParam)
{
	//��ȡ��ǰλ��
	//MT_Get_Axis_Software_P_Now(iID,&iP_Now);
	//��ȡ��ǰ�ٶ�
	//MT_Get_Axis_V_Now(iID,&iV_Now);
	//��ȡ��ǰ�˶�״̬
	//MT_Get_Axis_Status(iID,&iRun,&iDir,&iNeg,&iPos,&iZero,&iMode);

	//֪ͨ���߳�
	::PostMessage((HWND)(AfxGetMainWnd()->GetSafeHwnd()),WM_USER_IMAGE_ACQ,NULL,NULL);
}

BEGIN_MESSAGE_MAP(CMotorCtrl, CWinThread)
	ON_THREAD_MESSAGE(WM_USER_READ_MOTOR_STATUS,ReadMotorStatus)
END_MESSAGE_MAP()


// CMotorCtrl message handlers
