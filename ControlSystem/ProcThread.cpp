// ProcThread.cpp : implementation file
//

#include "stdafx.h"
#include "ControlSystem.h"
#include "ProcThread.h"
#include "IMotorCtrl.h"
#include "ImageProcess.h"
#include "ImageProcSettingDlg.h"
#include "ImageProcSetOblongDlg.h"
#include "ImageProcSetRectangleDlg.h"
#include "ImageProcSetLineDlg.h"
#include "DataUtility.h"
#include "AxialDeviationAngle.h"
#include "ImageProcSetAllDlg.h"
#include "DetectOblong.h"
#include <math.h>

// CProcThread

IMPLEMENT_DYNCREATE(CProcThread, CWinThread)

CProcThread::CProcThread()
: m_IMotoCtrl(NULL)
, m_IsMotroCtrlConnected(FALSE)
, m_IImageProcess(NULL)
, m_imageProcSetAllDlg(NULL)
, m_HalconWndOpened(false)
, m_pListData(NULL)
, m_MearTolerance(0.5)
, m_DistenceCameraAndTarget(80.0)
, m_AxialDeviationAngleDlg(NULL)
, m_ZMoveTopV(50.0f)
, m_XCalV(10.0f)
, m_YCalV(10.0f)
, m_workpieceType(DETECT_CIRCLE)
{
	//������
	m_DeviationAngle = (DataUtility::GetProfileFloat(_T("Axial Deviation Angle"), _T("Angle"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\SysConfig.ini")), 0.0f)) * 3.1415f / 180;
}

CProcThread::~CProcThread()
{
	
}

BOOL CProcThread::InitInstance()
{
	//��ʼ���忨
	m_IMotoCtrl = new IMotorCtrl();
	if(NULL != m_IMotoCtrl)
	{
		INT32 intResult = 0;
		intResult = m_IMotoCtrl->Init();
		if(0 != intResult)
		{
			AfxMessageBox(_T("���ƿ���ʼ��ʧ�ܣ�"));
		}
	}

	m_IImageProcess = new CImageProcess();

	m_MearTolerance = DataUtility::GetProfileFloat(_T("Car Frame"), _T("MearTolerance"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\SysConfig.ini")), 0.5f);
	m_DistenceCameraAndTarget = DataUtility::GetProfileFloat(_T("Distance Camera and Target"), _T("CTDistance"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\SysConfig.ini")), 80.0f);
	
	m_ZMoveTopV = DataUtility::GetProfileFloat(_T("Processing Motor V"), _T("ZMoveTopV"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\MTConfig.ini")), 50.0f);
	m_XCalV = DataUtility::GetProfileFloat(_T("Processing Motor V"), _T("XCalV"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\MTConfig.ini")), 10.0f);
	m_YCalV = DataUtility::GetProfileFloat(_T("Processing Motor V"), _T("YCalV"), (DataUtility::GetExePath() + _T("\\ProcessConfig\\MTConfig.ini")), 10.0f);

	return TRUE;
}

int CProcThread::ExitInstance()
{
	if(m_imageProcSetAllDlg != NULL)
	{
		delete m_imageProcSetAllDlg;
		m_imageProcSetAllDlg = NULL;
	}

	if(m_AxialDeviationAngleDlg != NULL)
	{
		m_AxialDeviationAngleDlg->DestroyWindow();
		delete m_AxialDeviationAngleDlg;
		m_AxialDeviationAngleDlg = NULL;
	}

	if(m_IImageProcess != NULL)
	{
		delete m_IImageProcess;
	}
	
	if(m_IMotoCtrl != NULL)
	{
		m_IMotoCtrl->CloseComPort();
		m_IMotoCtrl->DeInit();

		delete m_IMotoCtrl;
	}

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CProcThread, CWinThread)
	ON_THREAD_MESSAGE(WM_MOTOR_CONNECT,&CProcThread::OnMotorConnect)
	ON_THREAD_MESSAGE(WM_MOTOR_CLEAR_ZERO_X,&CProcThread::OnMotorClearZeroX)
	ON_THREAD_MESSAGE(WM_MOTOR_CLEAR_ZERO_Y,&CProcThread::OnMotorClearZeroY)
	ON_THREAD_MESSAGE(WM_MOTOR_CLEAR_ZERO_Z,&CProcThread::OnMotorClearZeroZ)
	ON_THREAD_MESSAGE(WM_MOTOR_MANUAL_START_MOVE,&CProcThread::OnMotorManualStartMove)
	ON_THREAD_MESSAGE(WM_MOTOR_MANUAL_STOP_MOVE,&CProcThread::OnMotorManualStopMove)
	ON_THREAD_MESSAGE(WM_MOTOR_GET_STATUS,&CProcThread::OnMotorGetStatus)
	ON_THREAD_MESSAGE(WM_MOTOR_STOP,&CProcThread::OnMotorStop)
	ON_THREAD_MESSAGE(WM_MOTOR_MOVE_TO,&CProcThread::OnMotorMoveTo)
	ON_THREAD_MESSAGE(WM_DO_CUSTOM_MEAR,&CProcThread::OnDoCustomMear)
	ON_THREAD_MESSAGE(WM_DO_AUTO_MEAR,&CProcThread::OnDoAutoMear)
	ON_THREAD_MESSAGE(WM_IMAGE_PROC,&CProcThread::OnDoImageProc)
	ON_THREAD_MESSAGE(WM_IMAGE_LOAD,&CProcThread::OnDoImageLoad)
	ON_THREAD_MESSAGE(WM_IMAGE_PROC_SETTING,&CProcThread::OnImageProcSetting)
	ON_THREAD_MESSAGE(WM_OPEN_AXIAL_DEVIATION_ANGLE_WND,&CProcThread::OnOpenCacAngleWnd)
END_MESSAGE_MAP()

void CProcThread::OnMotorConnect(WPARAM wParam,LPARAM lParam)
{
	m_IsMotroCtrlConnected = false;
	if(NULL != m_IMotoCtrl)
	{
		m_IMotoCtrl->CloseComPort();

		INT32 iResult = m_IMotoCtrl->OpenComPort();
		if(0 != iResult)
		{
			AfxMessageBox("���ӿ��ƿ�ʧ��!");
			return;
		}

		iResult = m_IMotoCtrl->Check();
		if(0 != iResult)
		{
			AfxMessageBox("�����ƿ�ʧ�ܣ�");
			return;
		}
		else
		{
			m_IsMotroCtrlConnected = true;
			AfxMessageBox("���ƿ����ӳɹ���");
		}
	}
}

void CProcThread::OnMotorClearZeroX(WPARAM wParam,LPARAM lParam)
{
	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		//float pos = *(float*)wParam;
		m_IMotoCtrl->SetAxisCurrPos(AXIS_X, 0);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

void CProcThread::OnMotorClearZeroY(WPARAM wParam,LPARAM lParam)
{
	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		//float pos = *(float*)wParam;
		m_IMotoCtrl->SetAxisCurrPos(AXIS_Y, 0);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

void CProcThread::OnMotorClearZeroZ(WPARAM wParam,LPARAM lParam)
{
	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		//float pos = *(float*)wParam;
		m_IMotoCtrl->SetAxisCurrPos(AXIS_Z, 0);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

void CProcThread::OnMotorManualStartMove(WPARAM wParam,LPARAM lParam)
{
	int axis = int(wParam);
	int dir = int(lParam);

	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		m_IMotoCtrl->SetAxisVelocityStart(axis, dir);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

void CProcThread::OnMotorManualStopMove(WPARAM wParam,LPARAM lParam)
{
	int axis = int(wParam);

	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		m_IMotoCtrl->SetAxisVelocityStop(axis);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

void CProcThread::OnMotorGetStatus(WPARAM wParam,LPARAM lParam)
{
	int axis = int(wParam);
	int status = int(lParam);

	static MotorStatus CurPos[AXIS_NUM];
	CurPos[axis].axis = (int)wParam;
	CurPos[axis].curPos = 0.0f;

	if(status == CURR_POS)
	{
		m_IMotoCtrl->GetAxisCurrPos(axis, &CurPos[axis].curPos);
		::PostMessage((HWND)(GetMainWnd()->GetSafeHwnd()), WM_MOTOR_UPDATE_STATUS, WPARAM(&CurPos[axis]), 0);
	}
}

void CProcThread::OnMotorStop(WPARAM wParam,LPARAM lParam)
{
	int axis = int(wParam);

	if(NULL != m_IMotoCtrl)
	{
		m_IMotoCtrl->SetAxisPositionStop(axis);
	}
}

void CProcThread::OnMotorMoveTo(WPARAM wParam,LPARAM lParam)
{
	int axis = int(wParam);
	float pos = float(lParam);

	if(NULL != m_IMotoCtrl && true == m_IsMotroCtrlConnected)
	{
		m_IMotoCtrl->MoveTo(axis, pos);
	}
	else
	{
		AfxMessageBox("���ƿ�δ���ӣ�");
	}
}

bool CProcThread::GetFloatItem(int row, int column, float &value)
{
	CString buffer=""; 
	if(NULL != m_pListData) buffer += m_pListData->GetItemText(row,column);
	return DataUtility::ConvertStringToFloat(buffer, value);
}

bool CProcThread::SetFloatItem(int row, int column, float value)
{
	CString buffer=""; 
	buffer.Format("%.2f", value);
	if(NULL != m_pListData)
	{
		m_pListData->SetItemText(row,column, buffer);
		return true;
	}
	else
	{
		return false;
	}
}

bool CProcThread::GetMeasureTargetValue(int row, float &x, float &y, float &z)
{
	if(GetFloatItem(row, COLUMN_POS_X, x))
	{
		if(GetFloatItem(row, COLUMN_POS_Y, y))
		{
			if(GetFloatItem(row, COLUMN_POS_Z, z))
			{
				return true;
			}
		}
	}

	return false;
}

void CProcThread::OnDoAutoMear(WPARAM wParam,LPARAM lParam)
{
	m_pListData = (CListCtrl*)wParam;
	int usedRowNum = (int)lParam;

	CDetectCircularhole* detecter = m_IImageProcess->GetCircleDetecter();
	if(detecter != NULL)
	{
		detecter->LoadConfig();
	}

	float x = 0.0, y = 0.0, z = 0.0;
	float retX = 0.0, retY = 0.0, retZ = 0.0;
	for(int i = ROW_START; i < usedRowNum; i = i + 3)
	{
		//��ȡ��������
		CString SType = m_pListData->GetItemText(i, COLUMN_MEAR_TYPE);
		if(SType == _T("Բ��"))
		{
			m_workpieceType = DETECT_CIRCLE;
		}
		else if(SType == _T("��Բ��"))
		{
			m_workpieceType = DETECT_OBLONG;
		}
		else if(SType == _T("������"))
		{
			m_workpieceType = DETECT_RECTANGLE;
		}
		else if(SType == _T("����Բ��"))
		{
			m_workpieceType = DETECT_SPECIAL_CIRCLE;
		}
		else
		{
			m_workpieceType = DETECT_CIRCLE;
		}

		if(GetMeasureTargetValue(i, x, y, z))
		{
			//����ֵ��	��װ�ĳߴ磬�������ڼ��Ŀ�λ����Ҫ���ӹ�װ
			//			����ʱ����λ�ߴ� + ����ֵ = ��װ�ߴ�
			//			���������󣬲������ - ����ֵ = ��λ�ߴ�
			//get compensation value
			float compensationX = 0.0f, compensationY = 0.0f, compensationZ = 0.0f;
			GetFloatItem(i, COLUMN_COMPENSATION_X, compensationX);
			GetFloatItem(i, COLUMN_COMPENSATION_Y, compensationY);
			GetFloatItem(i, COLUMN_COMPENSATION_Z, compensationZ);

			//��������ƫ��Ǽ���ʵ�����߳ߴ�
			if(0 == MoveToTargetPosXYZ((x + compensationX)*cos(m_DeviationAngle), (y + compensationY)*cos(m_DeviationAngle), -z - compensationZ, retX, retY, retZ))
			{
				
				//��������ƫ��Ǽ���ʵ�������
				SetFloatItem(i + 1, COLUMN_POS_X, (retX - compensationX) / cos(m_DeviationAngle));
				SetFloatItem(i + 1, COLUMN_POS_Y, (retY - compensationY) / cos(m_DeviationAngle));
				SetFloatItem(i + 1, COLUMN_POS_Z, -retZ + compensationZ);
			}
		}
	}

	AfxMessageBox("�Զ�������ɣ�");
}

int CProcThread::MoveToTargetPosXY(float x, float y, float z, float &retx, float &rety, bool calX/* = false*/, bool calY/* = false*/)
{
	int ret = 0;

	if(calX == true)
	{
		ret = m_IMotoCtrl->MoveTo(AXIS_X, x, m_XCalV);
	}
	else
	{
		ret = m_IMotoCtrl->MoveTo(AXIS_X, x);
	}
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_X) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

	if(calY == true)
	{
		ret = m_IMotoCtrl->MoveTo(AXIS_Y, y, m_YCalV);
	}
	else
	{
		ret = m_IMotoCtrl->MoveTo(AXIS_Y, y);
	}
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_Y) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

	//Ϊ��֤�빤���̶�m_DistenceCameraAndTargetλ�����㣬��Ҫ���ƶ�Z�ᣬʹ���빤������̶�m_DistenceCameraAndTarget
	ret = m_IMotoCtrl->MoveTo(AXIS_Z, z - m_DistenceCameraAndTarget);
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_Z) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

	//ͬ����Ϣ���ȴ����߳����ս��
	Sleep(500);
	ret = ::SendMessage((HWND)(GetMainWnd()->GetSafeHwnd()),WM_MAIN_THREAD_DO_CAPTURE, 0, 0);
	if(ret == 0)
	{
		OpenHalconWindow();
		m_IImageProcess->GetCircleDetecter()->ShowErrorMessage(false);
		m_IImageProcess->SetDetectType(m_workpieceType);
		if(m_IImageProcess->Process(x, y, retx, rety) == false)
		{
			return -1;
		}
	}

	return ret;
}

int CProcThread::MoveToTargetPosXYZ(float x, float y, float z, float &retx, float &rety, float &retz)
{
	int ret = 0;

#if 0	//for test
	//ret = ::SendMessage((HWND)(GetMainWnd()->GetSafeHwnd()),WM_MAIN_THREAD_DO_CAPTURE, 0, 0);
	//if(ret == 0)
	//{
	//	OpenHalconWindow();
	//	m_IImageProcess->GetCircleDetecter()->ShowErrorMessage(false);
	//	m_IImageProcess->Process(x, y, retx, rety);
	//}
	retx = x + 1;
	rety = y;
	retz = z;
#else

	//Z��ص�����λ���ش�
	ret = m_IMotoCtrl->SetAxisVelocityStart(AXIS_Z, 0, m_ZMoveTopV);
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_Z) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

	float difretx = 0.0, difrety = 0.0;
	ret = MoveToTargetPosXY(x, y, z, difretx, difrety);

	int procCount = 0;
	while(procCount < 2 && ret == 0)
	{
		m_IMotoCtrl->GetAxisCurrPos(AXIS_X, &retx);
		m_IMotoCtrl->GetAxisCurrPos(AXIS_Y, &rety);

		if((abs(difretx) > m_MearTolerance) || (abs(difrety) > m_MearTolerance))
		{
			if((abs(difretx) > m_MearTolerance) && (abs(difrety) > m_MearTolerance))
			{
				ret = MoveToTargetPosXY(retx - difretx, rety + difrety, z, difretx, difrety, true, true);
			}
			else if((abs(difretx) > m_MearTolerance) && (abs(difrety) <= m_MearTolerance))
			{
				ret = MoveToTargetPosXY(retx - difretx, rety, z, difretx, difrety, true, false);
			}
			else if((abs(difretx) <= m_MearTolerance) && (abs(difrety) > m_MearTolerance))
			{
				ret = MoveToTargetPosXY(retx, rety + difrety, z, difretx, difrety, false, true);
			}
		}
		else
		{
			break;
		}

		procCount++;
	}

	//X,Y�������������ȡ��ǰ������ֵ
	if(ret == 0)
	{
		m_IMotoCtrl->GetAxisCurrPos(AXIS_X, &retx);
		m_IMotoCtrl->GetAxisCurrPos(AXIS_Y, &rety);
	}
	else
	{
		return -1;
	}

	//Z�������ƶ���ֱ���Ӵ�����ֹͣ����ȡZ���ƶ��г�
	ret = m_IMotoCtrl->SetAxisVelocityStart(AXIS_Z, 1);
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_Z) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

	m_IMotoCtrl->GetAxisCurrPos(AXIS_Z, &retz);

	//Z��ص�����λ���ش�
	ret = m_IMotoCtrl->SetAxisVelocityStart(AXIS_Z, 0, m_ZMoveTopV);
	while(ret == 0)
	{
		if(m_IMotoCtrl->IsOnMoving(AXIS_Z) == false)
		{
			break;
		}
		else
		{
			Sleep(100);
		}
	}

#endif

	return ret;
}
void CProcThread::OnDoCustomMear(WPARAM wParam,LPARAM lParam)
{
	float* pPos = (float*)wParam;
	m_workpieceType = (int)lParam;
	float PosX = pPos[0];
	float PosY = pPos[1];
	float PosZ = pPos[2];
	float resPosX = 0.0;
	float resPosY = 0.0;
	float resPosZ = 0.0;
	CString msg;

	//��������ƫ��Ǽ���ʵ�����߳ߴ�
	PosX = PosX * cos(m_DeviationAngle);
	PosY = PosY * cos(m_DeviationAngle);
	if(0 == MoveToTargetPosXYZ(PosX, PosY, PosZ, resPosX, resPosY, resPosZ))
	{
		//success
		if((abs(resPosX - PosX) > m_MearTolerance) || (abs(resPosY - PosY) > m_MearTolerance) || (abs(resPosZ - PosZ) > m_MearTolerance))
		{
			msg.Format("�������: ���ϸ�\nͼֽ�ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.\nʵ��ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.", PosX, PosY, PosZ, resPosX, resPosY, resPosZ);
			AfxMessageBox(msg, MB_ICONERROR);
		}
		else
		{
			msg.Format("�������: �ϸ�\nͼֽ�ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.\nʵ��ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.", PosX, PosY, PosZ, resPosX, resPosY, resPosZ);
			AfxMessageBox(msg, MB_ICONINFORMATION );
		}

		//��������ƫ��Ǽ���������
		resPosX = resPosX / cos(m_DeviationAngle);
		resPosY = resPosY / cos(m_DeviationAngle);
	}
	else
	{
		msg.Format("�������: ���ϸ�\nͼֽ�ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.\nʵ��ߴ�: x = %.2f mm,  y = %.2f mm,  z = %.2f mm.", PosX, PosY, PosZ, resPosX, resPosY, resPosZ);
		AfxMessageBox(msg, MB_ICONERROR);
	}
}

void CProcThread::OnDoImageProc(WPARAM wParam,LPARAM lParam)
{
	OpenHalconWindow();

	if((NULL != m_IImageProcess) && m_IImageProcess->LoadProcessImage())
	{
		CDetectCircularhole* detecter = m_IImageProcess->GetCircleDetecter();
		if(detecter != NULL)
		{
			detecter->LoadConfig();
		}
		
		float x = 0.0, diffretx = 0.0, diffrety = 0.0;
		float y = 0.0;
		m_IImageProcess->SetDetectType(m_workpieceType);
		bool ret = m_IImageProcess->Process(x, y, diffretx, diffrety);
		if(!ret)
		{
			AfxMessageBox("Can not find target!");
		}
		else
		{
			CString msg;
			msg.Format("Բ�ĵ�ͼ������λ��: Dif X = %.2f mm,  Dif Y = %.2f mm.", diffretx, diffrety);
			AfxMessageBox(msg, MB_ICONINFORMATION );
		}
	}
}

void CProcThread::OnDoImageLoad(WPARAM wParam,LPARAM lParam)
{
	if(NULL != m_IImageProcess)
	{
		m_IImageProcess->LoadProcessImage();
	}
}

void CProcThread::OnImageProcSetting(WPARAM wParam,LPARAM lParam)
{
	OpenHalconWindow();
	
	if(NULL != m_IImageProcess && m_IImageProcess->LoadProcessImage())
	{
		if(m_imageProcSetAllDlg == NULL)
		{
			m_imageProcSetAllDlg = new CImageProcSetAllDlg();
			m_imageProcSetAllDlg->Create(CImageProcSetAllDlg::IDD, GetMainWnd());
		}

		CDetectCircularhole* circleDetecter = m_IImageProcess->GetCircleDetecter();
		if(circleDetecter != NULL) m_imageProcSetAllDlg->m_ImageProcSetDlg->SetCircleDetecter(circleDetecter);

		CDetectOblong* oblongDetecter = m_IImageProcess->GetOblongDetecter();
		if(oblongDetecter != NULL) m_imageProcSetAllDlg->m_imageProcSetOblongDlg->SetDetecter(oblongDetecter);

		CDetectRectangle* rectangleDetecter = m_IImageProcess->GetRectangleDetecter();
		if(rectangleDetecter != NULL) m_imageProcSetAllDlg->m_imageProcSetRectangleDlg->SetDetecter(rectangleDetecter);

		/*CDetectLine* lineDetecter = m_IImageProcess->GetLineDetecter();
		if(lineDetecter != NULL) m_imageProcSetAllDlg->m_imageProcSetLineDlg->SetDetecter(lineDetecter);*/
		CDetectCircularhole* specialCircleDetecter = m_IImageProcess->GetSpecialCircleDetecter();
		if(specialCircleDetecter != NULL) m_imageProcSetAllDlg->m_ImageProcSetSmallCircleDlg->SetCircleDetecter(specialCircleDetecter);

		m_imageProcSetAllDlg->ShowWindow(SW_SHOW);
	}
}

void CProcThread::OnOpenCacAngleWnd(WPARAM wParam,LPARAM lParam)
{
	if(m_AxialDeviationAngleDlg == NULL)
	{
		m_AxialDeviationAngleDlg = new CAxialDeviationAngle();
	}
		
	if(m_AxialDeviationAngleDlg->GetSafeHwnd() == NULL)
	{
		m_AxialDeviationAngleDlg->Create(IDD_AXIAL_DEVIATION_ANGLE, NULL);
		m_AxialDeviationAngleDlg->Invalidate();
		m_AxialDeviationAngleDlg->SetParent(this);
	}
	m_AxialDeviationAngleDlg->ShowWindow(TRUE);
}

void CProcThread::OpenHalconWindow()
{
	if(m_HalconWndOpened)
	{
		return;
	}

	Halcon::set_window_attr("background_color","black");

	Halcon::open_window(0, 0, 640, 480, 0, "","",&m_WindowHandle);

	HDevWindowStack::Push(m_WindowHandle);

	m_HalconWndOpened = true;
}

// CProcThread message handlers
