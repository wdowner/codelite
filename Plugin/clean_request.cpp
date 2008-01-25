#include "clean_request.h"
#include "buildmanager.h"
#include "wx/process.h"
#include "dirsaver.h"
#include "workspace.h"
 
CleanRequest::CleanRequest(wxEvtHandler *owner, const wxString &projectName, bool projectOnly)
: CompilerAction(owner)
, m_project(projectName)
, m_projectOnly(projectOnly)
{
	
} 
 
CleanRequest::~CleanRequest()
{
	//no need to delete the process, it will be deleted by the wx library
}


//do the actual cleanup
void CleanRequest::Process()
{
	wxString cmd;
	wxString errMsg;
	SetBusy(true);
	
	ProjectPtr proj = WorkspaceST::Get()->FindProjectByName(m_project, errMsg);
	if (!proj) {
		AppendLine(wxT("Cant find project: ") + m_project);
		SetBusy(false);
		return;
	}
	
	bool isCustom(false);
	//TODO:: make the builder name configurable
	BuilderPtr builder = BuildManagerST::Get()->GetBuilder(wxT("GNU makefile for g++/gcc"));
	if(m_projectOnly){
		cmd = builder->GetPOCleanCommand(m_project, isCustom);
	}else{
		cmd = builder->GetCleanCommand(m_project, isCustom);
	}

	SendStartMsg();
	m_proc = new clProcess(wxNewId(), cmd);
	
	if(m_proc) {
		
		DirSaver ds;
		
		if(isCustom){
			//first set the path to the project working directory
			::wxSetWorkingDirectory(proj->GetFileName().GetPath());
			BuildConfigPtr buildConf = WorkspaceST::Get()->GetProjSelBuildConf(m_project);
			if(buildConf) {
				wxString wd = buildConf->GetCustomBuildWorkingDir();
				if(wd.IsEmpty()) {
					wd = proj->GetFileName().GetPath();
				}
				
				AppendLine(wxT("Changing working directory to: ") + wd + wxT("\n"));
				::wxSetWorkingDirectory(wd);
				AppendLine(wxT("Current working directory is: ") + wxGetCwd() + wxT("\n"));
			}
		}
		
		if(m_projectOnly ){
			//need to change directory to project dir
			wxSetWorkingDirectory(proj->GetFileName().GetPath());
		}
		
		if(m_proc->Start() == 0){
			wxString message;
			message << wxT("Failed to start clean process, command: ") << cmd << wxT(", process terminated with exit code: 0");
			AppendLine(message);
			SetBusy(false);
			delete m_proc;
			return;
		}
		
		Connect(wxEVT_TIMER, wxTimerEventHandler(CleanRequest::OnTimer), NULL, this);
		m_proc->Connect(wxEVT_END_PROCESS, wxProcessEventHandler(CleanRequest::OnProcessEnd), NULL, this);
		m_timer->Start(10);
	}
}
