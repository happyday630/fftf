/*----------------------------------------------------------
 | Linux based APIs Auto Test Framework
 | ----------------------------------------------------------
 | QA test frame which managing tests and control cases running.
 |
 | File Name: 		frame.c
 | Author:		Liu, Yang
 | Created date: 	11/03/2011
 | Version: 		v_0_1
 | Last update: 	03/20/2012
 | Updated by: 	liuyang
 +----------------------------------------------------------*/

/*----------------------------------------------------------
 | Include definition below this line
 +----------------------------------------------------------*/
#include <sys/types.h>  
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include "frame.h"


/*----------------------------------------------------------
 | Macro definition below this line
 +----------------------------------------------------------*/
#define MAX_NAME_LEN	64

/*----------------------------------------------------------
 | Type definition below this line
 +----------------------------------------------------------*/

/*Case control running structs*/
typedef struct TestResult
{
	struct TestModule	*pTestModule;		/*Pointer to test module*/
	struct TestSuite	*pTestSuite;		/*Pointer to test suite*/
	struct TestCase 	*pTestCase;		/*Pointer to test case*/
	guint			uiTimesOfPass;		/*Number of running times*/
	guint			uiTimesOfFail;		/*Number of Fail times*/
	guint 			uiTimesOfCrash;		/*Number of Crash times*/
	guint    		uiTimesOfNoRan;		/*Number of No Run times*/
	gint			iSubCaseWeight;		/*The number used to randly select*/
	QaError			ExceptionCode;		/*Exception code*/
	struct TestResult	*pPrev;			/*Pointer to previous test result record in list*/
	struct TestResult	*pNext;			/*Pointer to next test result record in list*/
}TestResult;

typedef enum
{
	STATUS_STOPPED = 1,
	STATUS_RUNNING,
	STATUS_PAUSED
}CaseRunningStatus;

typedef struct FrameStatus
{
	struct TestResult	*pCurrentResult;		/*pointer to current running test case's result*/
	struct TestResult	*pTestResult;			/*pointer to first test result record*/
	gint				iNumOfRemainingCases;	/*The number of remaining cases need to run*/
	gint				iNumOfRandomSteps;	/*The number of random stress steps*/
	gint			iSumOfCaseWeight;		/*The sum of cases' weight*/
	CaseRunningStatus		iStatus;		/*Stopped, running, or Pause*/
	RunningMode	iRMode;					/*The running mode of current testing*/
}FrameStatus;

typedef struct PipeData
{
	gchar				module_name[MAX_NAME_LEN+1];
	gchar				suite_name[MAX_NAME_LEN+1];
	gchar				case_name[MAX_NAME_LEN+1];
	CaseResult			iResult;
	QaError				iFrameError;
}PipeData;

typedef void (*case_driver)(gint);


/*----------------------------------------------------------
 | Global vars definition below this line
 +----------------------------------------------------------*/
static TestRegistry *g_pTestRegistry;
static FrameStatus *g_pFrameStatus;
static guint g_uiCaseIntervalTime = 2;
static pid_t case_driver_pid;
static gint pipe_fd[2];

/*----------------------------------------------------------
 | Functions prototype below this line
 +----------------------------------------------------------*/
TestCase *qa_create_test_case(const gchar * pCaseName,TestFunc pTestFunc);
TestCase *qa_find_case_in_suite(TestSuite * pTestSuite,const gchar * pCaseName,QaError * ErrorCode);
void insert_test_case(TestRegistry * pTestRegistry,TestSuite * pTestSuite,TestCase * pTestCase);
void cleanup_test_case(TestCase * pTestCase);
TestSuite *qa_create_test_suite(const gchar * pSuiteName,InitFunc pInitFunc,CleanupFunc pCleanupFunc);
TestSuite *qa_find_suite(TestModule * pTestModule,const gchar * pSuiteName,QaError * ErrorCode);
void insert_test_suite(TestRegistry * pTestRegistry,TestModule * pTestModule,TestSuite * pTestSuite);
void cleanup_test_suite(TestSuite * pTestSuite);
TestModule *qa_create_test_module(const gchar * pName);
TestModule *qa_find_module(TestRegistry * pTestRegistry,const gchar * pName,QaError * ErrorCode);
void insert_test_module(TestRegistry * pTestRegistry,TestModule * pTestModule);
void cleanup_test_module(TestModule * pTestModule);
void cleanup_test_registry(TestRegistry * pTestRegistry);
void qa_destroy_existing_registry(TestRegistry * * ppTestRegistry);
void qa_clear_previous_results(void);
void qa_frame_cleanup_registry(void);
TestRegistry *qa_create_new_registry(void);
QaError qa_frame_initialize_test_registry(void);
guint qa_frame_get_total_modules(void);
guint qa_frame_get_total_suites(void);
guint qa_frame_get_total_cases(void);
QaError push_case_to_result_list(TestModule * pTestModule,TestSuite * pTestSuite,TestCase * pTestCase,FrameStatus * pFrameStatus);
QaError push_suite_to_result_list(TestModule * pTestModule,TestSuite * pTestSuite,FrameStatus * pFrameStatus, int priority);
QaError push_module_to_result_list(TestModule * pTestModule,FrameStatus * pFrameStatus, int priority);
QaError push_case_to_result_list_by_ids(char *case_list, int priority);
void send_test_result(gchar * module_name,gchar * suite_name,gchar * case_name,CaseResult result,QaError ErrorCode,gint write_pipe);
gint read_test_result(PipeData * data,gint read_pipe);
TestResult *find_test_result(gchar * module_name,gchar * suite_name,gchar * case_name);
QaError do_test_suite_init(TestSuite * pTestSuite);
CaseResult run_single_case(TestResult * pTestResult,QaError * error);
void do_test_suite_cleanup(TestSuite * pTestSuite);
QaError qa_frame_print_out_test_result(void);
FrameStatus* qa_create_new_frame_status(void);
void sequence_goto_next_suite(FrameStatus * pFrameStatus,gint write_pipe);
void sequence_case_driver(gint write_pipe);
pid_t fork_child_process(case_driver driver_f);
void signal_handler(int sig);
QaError qa_do_sequence_testing(void);
void random_goto_next_case(FrameStatus * pFrameStatus);
void random_case_driver(gint write_pipe);
QaError qa_do_random_testing(void);
void set_test_result(TestResult * pTestResult,CaseResult result,QaError ErrorCode);


/*----------------------------------------------------------
 | Functions definition below this line
 +----------------------------------------------------------*/
TestCase* qa_create_test_case(const gchar *pCaseName, TestFunc pTestFunc)
{
	TestCase *pRetCase = NULL;
	gint len;
	
	pRetCase = (TestCase *)g_slice_alloc0(sizeof(TestCase));

	if(pRetCase != NULL){
		len = strlen(pCaseName);
		if(len>MAX_NAME_LEN)
			len = MAX_NAME_LEN;
		pRetCase->pName = (gchar *)g_slice_alloc0(len + 1);
		if(pRetCase->pName != NULL){
			strncpy(pRetCase->pName, pCaseName, len);
			pRetCase->pTestFunc = pTestFunc;
			pRetCase->iCaseWight = CASE_WEIGHT_MEDIUM;
			pRetCase->pPrev = NULL;
			pRetCase->pNext = NULL;
			pRetCase->uiID = 0;
		}else{
			g_slice_free1(sizeof(TestCase), pRetCase);
			pRetCase = NULL;
		}
	}

	return pRetCase;
}

TestCase* qa_find_case_in_suite(TestSuite *pTestSuite, const gchar *pCaseName, QaError *ErrorCode)
{
	TestCase *pCurCase = NULL;
	*ErrorCode = FRAME_NO_CASE;

	pCurCase = pTestSuite->pTestCase;
	while(pCurCase != NULL){
		if(strcmp(pCurCase->pName, pCaseName)==0){
			*ErrorCode = FRAME_SUCCESS;
			break;
		}

		pCurCase = pCurCase->pNext;
	}

	return pCurCase;
	
}

TestCase* qa_frame_get_case_by_name(const gchar *pCaseName, QaError *ErrorCode)
{
	TestModule *pModule=NULL;
	TestSuite  *pSuite=NULL;
	TestCase   *pCase=NULL;
	
	*ErrorCode = FRAME_NO_CASE;

	if(g_pTestRegistry==NULL){
		*ErrorCode = FRAME_NO_REGISTRY;
		return NULL;
	}else if((pModule=g_pTestRegistry->pTestModule)==NULL){
		*ErrorCode = FRAME_NO_MODULE;
		return NULL;
	}else if((pSuite=pModule->pTestSuite)==NULL){
		*ErrorCode = FRAME_NO_SUITE;
		return NULL;
	}else if((pCase=pSuite->pTestCase)==NULL){
		*ErrorCode = FRAME_NO_CASE;
		return NULL;
	}

	while(pModule!=NULL){
		while(pSuite!=NULL){
			while(pCase!=NULL){
				if(g_strcmp0(pCase->pName, pCaseName)==0){
					*ErrorCode = FRAME_SUCCESS;
					return pCase;
				}
				pCase = pCase->pNext;
			}
			pSuite = pSuite->pNext;
		}
		pModule = pModule->pNext;
	}

	return NULL;
}

void insert_test_case(TestRegistry *pTestRegistry, TestSuite *pTestSuite, TestCase *pTestCase)
{
	TestCase *pCurCase = NULL;
	pCurCase = pTestSuite->pTestCase;

	pTestCase->pNext = NULL;
	pTestRegistry->uiNumberOfTotalTestCases++;
	pTestSuite->uiNumberOfTest++;
	pTestCase->uiID = pTestSuite->uiNumberOfTest;

	if(pCurCase == NULL){ /*first case*/
		pTestSuite->pTestCase = pTestCase;
		pTestCase->pPrev = NULL;
	}else{
		while(pCurCase->pNext != NULL)
			pCurCase = pCurCase->pNext;

		pCurCase->pNext = pTestCase;
		pTestCase->pPrev = pCurCase;
	}
}

void cleanup_test_case(TestCase *pTestCase)
{
	if(pTestCase->pName != NULL)
		g_slice_free1(strlen(pTestCase->pName)+1, pTestCase->pName);

	pTestCase = NULL;
}

TestCase* qa_frame_add_test_case(TestSuite *pTestSuite, const gchar *pCaseName, TestFunc pTestFunc, QaError *ErrorCode)
{
	TestCase *pRetCase=NULL;

	if(pTestSuite == NULL)
		*ErrorCode = FRAME_NO_SUITE;
	else if(pCaseName == NULL)
		*ErrorCode = FRAME_NO_CASE_NAME;
	else if(pTestFunc == NULL)
		*ErrorCode = FRAME_NO_CASE_FUNC;
	else if(qa_find_case_in_suite(pTestSuite, pCaseName, ErrorCode) != NULL)
		*ErrorCode = FRAME_DUPLICATE_CASE;
	else{
		pRetCase = qa_create_test_case(pCaseName, pTestFunc);

		if(pRetCase == NULL)
			*ErrorCode = FRAME_NO_MEMORY;
		else{
			insert_test_case(g_pTestRegistry, pTestSuite, pRetCase);
			*ErrorCode = FRAME_SUCCESS;
		}
	}

	return pRetCase;
}

TestSuite* qa_create_test_suite(const gchar *pSuiteName, InitFunc pInitFunc, CleanupFunc pCleanupFunc)
{
	TestSuite *pRetSuite;
	gint len;
	
	pRetSuite = (TestSuite *)g_slice_alloc0(sizeof(TestSuite));

	if(pRetSuite != NULL){
		len = strlen(pSuiteName);
		if(len>MAX_NAME_LEN)
			len = MAX_NAME_LEN;
		pRetSuite->pName = (gchar *)g_slice_alloc0(len+1);
		if(pRetSuite->pName != NULL){
			strncpy(pRetSuite->pName, pSuiteName, len);
			pRetSuite->pInitFunc = pInitFunc;
			pRetSuite->pCleanupFunc = pCleanupFunc;
			pRetSuite->pTestCase = NULL;
			pRetSuite->uiNumberOfTest = 0;
			pRetSuite->pNext = NULL;
			pRetSuite->pPrev = NULL;
			pRetSuite->uiID = 0;
		}else{
			g_slice_free1(sizeof(TestSuite), pRetSuite);
			pRetSuite = NULL;
		}
	}

	return pRetSuite;
}

TestSuite* qa_find_suite(TestModule *pTestModule, const gchar *pSuiteName, QaError *ErrorCode)
{
	TestSuite *pCurSuite = NULL;
	*ErrorCode = FRAME_NO_SUITE;

	pCurSuite = pTestModule->pTestSuite;

	while(pCurSuite != NULL){
		if(strcmp(pCurSuite->pName, pSuiteName) == 0){
			*ErrorCode = FRAME_SUCCESS;
			break;
		}

		pCurSuite = pCurSuite->pNext;
	}
	
	return pCurSuite;
}

TestSuite* qa_frame_get_suite_by_name(TestModule *pTestModule, const gchar *pSuiteName, QaError *ErrorCode)
{
	return qa_find_suite(pTestModule, pSuiteName, ErrorCode);
}

void insert_test_suite(TestRegistry *pTestRegistry, TestModule *pTestModule, TestSuite *pTestSuite)
{
	TestSuite *pCurSuite=NULL;
	pCurSuite = pTestModule->pTestSuite;

	pTestSuite->pNext = NULL;
	pTestModule->uiNumberOfSuite++;
	pTestRegistry->uiNumberOfTotalTestSuites++;
	pTestSuite->uiID = pTestModule->uiNumberOfSuite;

	if(pCurSuite == NULL){ /*first suite*/
		pTestModule->pTestSuite = pTestSuite;
		pTestSuite->pPrev = NULL;
	}else{
		while(pCurSuite->pNext != NULL)
			pCurSuite = pCurSuite->pNext;
		
		pCurSuite->pNext = pTestSuite;
		pTestSuite->pPrev = pCurSuite;
	}
}

void cleanup_test_suite(TestSuite *pTestSuite)
{
	TestCase *pCurCase;
	TestCase *pNextCase;

	pCurCase = pTestSuite->pTestCase;
	while(pCurCase != NULL){
		pNextCase = pCurCase->pNext;

		cleanup_test_case(pCurCase);
		g_slice_free1(sizeof(TestCase), pCurCase);

		pCurCase = pNextCase;
	}

	if(pTestSuite->pName != NULL)
		g_slice_free1(strlen(pTestSuite->pName)+1, pTestSuite->pName);

	pTestSuite->pName = NULL;
	pTestSuite->pTestCase = NULL;
	pTestSuite->uiNumberOfTest = 0;
	pTestSuite->uiID = 0;
}

TestSuite* qa_frame_add_test_suite(TestModule *pModule, const gchar* pSuiteName, InitFunc pInitFunc, CleanupFunc pCleanupFunc, QaError *ErrorCode)
{
	TestSuite *pRetSuite = NULL;

	if(pModule == NULL)
		*ErrorCode = FRAME_NO_MODULE;
	else if(pSuiteName == NULL)
		*ErrorCode = FRAME_NO_SUITE_NAME;
	else if(g_pTestRegistry == NULL)
		*ErrorCode = FRAME_NO_REGISTRY;
	else if(qa_find_suite(pModule, pSuiteName, ErrorCode) != NULL)
		*ErrorCode = FRAME_DUPLICATE_SUITE;
	else{
		pRetSuite = qa_create_test_suite(pSuiteName, pInitFunc, pCleanupFunc);
		if(pRetSuite == NULL)
			*ErrorCode = FRAME_NO_MEMORY;
		else{
			insert_test_suite(g_pTestRegistry, pModule, pRetSuite);
			*ErrorCode =  FRAME_SUCCESS;		
		}
	}

	return pRetSuite;
}

TestModule *qa_create_test_module(const gchar* pName)
{
	TestModule *pTestModule;
	gint len;
	
	pTestModule = (TestModule *)g_slice_alloc0(sizeof(TestModule));

	if(pTestModule != NULL){
		len = strlen(pName);
		if(len>MAX_NAME_LEN)
			len = MAX_NAME_LEN;		
		pTestModule->pName = (gchar *)g_slice_alloc0(len+1);
		if(pTestModule->pName != NULL){
			strncpy(pTestModule->pName, pName, len);
			pTestModule->pTestSuite = NULL;
			pTestModule->uiNumberOfSuite = 0;
			pTestModule->pPrev = NULL;
			pTestModule->pNext = NULL;
			pTestModule->uiID = 0;
		}else{
			g_slice_free1(sizeof(TestModule), pTestModule);
			pTestModule = NULL;
		}
	}

	return pTestModule;
}

TestModule* qa_find_module(TestRegistry *pTestRegistry, const gchar *pName, QaError *ErrorCode)
{
	TestModule *pCurModule = NULL;
	*ErrorCode = FRAME_NO_MODULE;

	pCurModule = pTestRegistry->pTestModule;

	while(pCurModule != NULL){
		if(strcmp(pCurModule->pName, pName) == 0){
			*ErrorCode = FRAME_SUCCESS;
			break;
		}

		pCurModule = pCurModule->pNext;
	}
	
	return pCurModule;
}

TestModule* qa_frame_get_module_by_name(const gchar *pName, QaError *ErrorCode)
{
	return qa_find_module(g_pTestRegistry, pName, ErrorCode);
}

TestModule* qa_frame_get_first_module(QaError *ErrorCode)
{	
	if(g_pTestRegistry == NULL){		
		*ErrorCode = FRAME_NO_REGISTRY;		
		return NULL;	
	}else if(g_pTestRegistry->pTestModule == NULL){		
		*ErrorCode = FRAME_NO_MODULE;		
		return NULL;	
	}else{		
		*ErrorCode = FRAME_SUCCESS;		
		return g_pTestRegistry->pTestModule;	
	}
}

void insert_test_module(TestRegistry *pTestRegistry, TestModule *pTestModule)
{
	TestModule *pCurModule = NULL;

	pCurModule = pTestRegistry->pTestModule;

	pTestModule->pNext = NULL;
	pTestRegistry->uiNumberOfTotalTestModules++;
	pTestModule->uiID = pTestRegistry->uiNumberOfTotalTestModules;

	if(pCurModule == NULL){/*The first module*/
		pTestRegistry->pTestModule = pTestModule;
		pTestModule->pPrev = NULL;
	}else{
		while(pCurModule->pNext != NULL){
			pCurModule = pCurModule->pNext;
		}
		pCurModule->pNext = pTestModule;
		pTestModule->pPrev = pCurModule;
	}
}

void cleanup_test_module(TestModule *pTestModule)
{
	TestSuite *pCurSuite;
	TestSuite *pNextSuite;

	pCurSuite = pTestModule->pTestSuite;
	while(pCurSuite != NULL){
		pNextSuite = pCurSuite->pNext;

		cleanup_test_suite(pCurSuite);
		g_slice_free1(sizeof(TestSuite), pCurSuite);

		pCurSuite = pNextSuite;
	}

	if(pTestModule->pName != NULL)
		g_slice_free1(strlen(pTestModule->pName)+1, pTestModule->pName);

	pTestModule->pName = NULL;
	pTestModule->pTestSuite = NULL;
	pTestModule->uiNumberOfSuite = 0;
	pTestModule->uiID = 0;
}

TestModule* qa_frame_add_test_module(const gchar* pName, QaError *ErrorCode)
{
	TestModule *retModule = NULL;

	if(pName == NULL){
		*ErrorCode = FRAME_NO_MODULE_NAME;
	}else if(g_pTestRegistry == NULL){
		*ErrorCode = FRAME_NO_REGISTRY;
	}else if(qa_find_module(g_pTestRegistry, pName, ErrorCode) != NULL){
		*ErrorCode = FRAME_DUPLICATE_MODULE;
	}else{

		retModule = qa_create_test_module(pName);
		if(retModule == NULL)
			*ErrorCode = FRAME_NO_MEMORY;
		else{
			insert_test_module(g_pTestRegistry, retModule);
			*ErrorCode =  FRAME_SUCCESS;
		}
	}

	return retModule;	
}


void cleanup_test_registry(TestRegistry *pTestRegistry)
{
	TestModule *pCurModule;
	TestModule *pNextModule;

	pCurModule = pTestRegistry->pTestModule;
	while(pCurModule != NULL){
		pNextModule = pCurModule->pNext;

		cleanup_test_module(pCurModule);
		g_slice_free1(sizeof(TestModule), pCurModule);
		
		pCurModule = pNextModule;
	}

	pTestRegistry->pTestModule = NULL;
	pTestRegistry->uiNumberOfTotalTestCases = 0;
	pTestRegistry->uiNumberOfTotalTestModules = 0;
	pTestRegistry->uiNumberOfTotalTestSuites = 0;
}

void qa_destroy_existing_registry(TestRegistry **ppTestRegistry)
{
	if(NULL != *ppTestRegistry){
		cleanup_test_registry(*ppTestRegistry);
	}

	g_slice_free1(sizeof(TestRegistry), *ppTestRegistry);
	*ppTestRegistry = NULL;
}

void qa_clear_previous_results(void)
{
	TestResult *pCurResult, *pNextResult;
	if(g_pFrameStatus != NULL){
		g_pFrameStatus->pCurrentResult= NULL;
		g_pFrameStatus->iNumOfRemainingCases = 0;
		g_pFrameStatus->iStatus= STATUS_STOPPED;
		g_pFrameStatus->iNumOfRandomSteps = 0;
		g_pFrameStatus->iRMode = RUNNING_SEQUENCE;
		g_pFrameStatus->iSumOfCaseWeight = 0;

		pCurResult = g_pFrameStatus->pTestResult;
		while(pCurResult != NULL){
			pNextResult = pCurResult->pNext;
			g_slice_free1(sizeof(TestResult), pCurResult);
			pCurResult = pNextResult;
		}
		g_pFrameStatus->pTestResult = NULL;

	}
}

void qa_frame_cleanup_registry(void)
{
  	qa_destroy_existing_registry(&g_pTestRegistry);
  	qa_clear_previous_results();
	if(g_pFrameStatus!=NULL){
		g_slice_free1(sizeof(FrameStatus), g_pFrameStatus);
		g_pFrameStatus = NULL;
	}
}

TestRegistry* qa_create_new_registry(void)
{
	TestRegistry *pTestRegistry = (TestRegistry *)g_slice_alloc0(sizeof(TestRegistry));
	if(NULL != pTestRegistry){
		pTestRegistry->pTestModule = NULL;
		pTestRegistry->uiNumberOfTotalTestCases = 0;
		pTestRegistry->uiNumberOfTotalTestModules = 0;
		pTestRegistry->uiNumberOfTotalTestSuites = 0;
	}

	return pTestRegistry;
}

QaError qa_frame_initialize_test_registry(void)
{
	QaError result = FRAME_SUCCESS;

	if (NULL != g_pTestRegistry) {
		qa_frame_cleanup_registry();
	}

	g_pTestRegistry = qa_create_new_registry();
	if (NULL == g_pTestRegistry) {
		result = FRAME_NO_MEMORY;
	}

	return result;
}

guint qa_frame_get_total_modules(void)
{
	if(g_pTestRegistry != NULL)
		return g_pTestRegistry->uiNumberOfTotalTestModules;
	else
		return 0;
}

guint qa_frame_get_total_suites(void)
{
	if(g_pTestRegistry != NULL)
		return g_pTestRegistry->uiNumberOfTotalTestSuites;
	else
		return 0;
}

guint qa_frame_get_total_cases(void)
{
	if(g_pTestRegistry != NULL)
		return g_pTestRegistry->uiNumberOfTotalTestCases;
	else
		return 0;
}

const gchar *qa_frame_get_error_str(QaError ErrorCode)
{
	static const gchar *qa_frame_error_desc[]=
	{
		"FRAME_SUCCESS",
		"FRAME_FAILURE",
		"FRAME_NO_MEMORY",
		"FRAME_NO_SUITE_NAME",
		"FRAME_NO_REGISTRY",
		"FRAME_DUPLICATE_MODULE",
		"FRAME_NO_MODULE",
		"FRAME_NO_SUITE_NAME",
		"FRAME_NO_SUITE",
		"FRAME_DUPLICATE_SUITE",
		"FRAME_NO_CASE_NAME",
		"FRAME_NO_CASE",
		"FRAME_DUPLICATE_CASE",
		"FRAME_NO_CASE_FUNC",
		"FRAME_INIT_SUITE_FAIL",
		"FRAME_NO_FRAME_STATUS",
		"FRAME_TIMER_CREATE_ERROR",
		"FRAME_TIMER_START_ERROR",
		"FRAME_NO_VERIF_FUNC",
		"FRAME_NO_RESULT_RECORD",
		"FRAME_ALREADY_START",
		"Undefined Error",
	};
	gint iMaxIndex = (gint)(sizeof(qa_frame_error_desc)/sizeof(gchar *)-1);
	gint iErrorIndex = (gint)(ErrorCode * (-1));
	
	if(iErrorIndex<0)
		return qa_frame_error_desc[0];
	else if(iErrorIndex>iMaxIndex)
		return qa_frame_error_desc[iMaxIndex];
	else
		return qa_frame_error_desc[iErrorIndex];
}

QaError push_case_to_result_list(TestModule *pTestModule, TestSuite *pTestSuite, TestCase *pTestCase, FrameStatus*pFrameStatus)
{
	QaError RetVal = FRAME_SUCCESS;
	TestResult *pCurResult = NULL;
	TestResult *pNewResult = NULL;

	QA_PRINT(3, ("push_case_to_result_list\n"));

	if(pTestSuite == NULL)
		RetVal = FRAME_NO_SUITE;
	else if(pTestCase == NULL)
		RetVal = FRAME_NO_CASE;
	else if(pTestModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(pFrameStatus == NULL)
		RetVal = FRAME_NO_FRAME_STATUS;
	else{
		pNewResult = (TestResult *)g_slice_alloc0(sizeof(TestResult));
		if(pNewResult == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			pNewResult->pTestModule = pTestModule;
			pNewResult->pTestSuite = pTestSuite;
			pNewResult->pTestCase = pTestCase;
			pNewResult->uiTimesOfPass = 0;
			pNewResult->uiTimesOfFail = 0;
			pNewResult->uiTimesOfNoRan = 0;
			pNewResult->uiTimesOfCrash = 0;
			pNewResult->ExceptionCode = FRAME_SUCCESS;
			pNewResult->pNext = NULL;
			pFrameStatus->iSumOfCaseWeight += pTestCase->iCaseWight;
			pNewResult->iSubCaseWeight = pFrameStatus->iSumOfCaseWeight;

			pCurResult = pFrameStatus->pTestResult;

			QA_PRINT(3, ("Insert test result to list. current_test=%s\n", pNewResult->pTestCase->pName));
			if(pCurResult == NULL){ /*first one*/
				pFrameStatus->pTestResult = pNewResult;
				pNewResult->pPrev = NULL;
			}else{
				while(pCurResult->pNext != NULL)
					pCurResult = pCurResult->pNext;
				pCurResult->pNext = pNewResult;
				pNewResult->pPrev = pCurResult;
			}

			pFrameStatus->iNumOfRemainingCases++;
		}
	}

	return RetVal;
}

QaError push_suite_to_result_list(TestModule *pTestModule, TestSuite *pTestSuite, FrameStatus*pFrameStatus, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	TestCase *pCurCase;

	QA_PRINT(3, ("push_suite_to_result_list\n"));

	if(pTestSuite == NULL)
		RetVal = FRAME_NO_SUITE;
	else if(pFrameStatus == NULL)
		RetVal = FRAME_NO_FRAME_STATUS;
	else{
		pCurCase = pTestSuite->pTestCase;
		while(pCurCase != NULL){
			if((pCurCase->iCaseWight)&priority){
				RetVal = push_case_to_result_list(pTestModule, pTestSuite, pCurCase, pFrameStatus);
				if(RetVal != FRAME_SUCCESS)
					printf("---Add test case '%s' to running list faile. ErrorCode='%s'.\n", pCurCase->pName, qa_frame_get_error_str(RetVal));
				else
					printf("---Add test case '%s' to running list Success.\n", pCurCase->pName);
			}else
				printf("***%s's priority=%d, ignored.\n", pCurCase->pName, pCurCase->iCaseWight);
			pCurCase = pCurCase->pNext;
		}
	}

	return RetVal;
}

QaError push_module_to_result_list(TestModule *pTestModule, FrameStatus*pFrameStatus, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	TestSuite *pCurSuite;

	QA_PRINT(3, ("push_module_to_result_list\n"));

	if(pTestModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(pFrameStatus == NULL)
		RetVal = FRAME_NO_FRAME_STATUS;
	else{
		pCurSuite = pTestModule->pTestSuite;
		while(pCurSuite!=NULL){
			RetVal = push_suite_to_result_list(pTestModule, pCurSuite, pFrameStatus, priority);
			if(RetVal != FRAME_SUCCESS)
				printf("-Add test suite '%s' to running list faile. ErrorCode='%s'.\n", pCurSuite->pName, qa_frame_get_error_str(RetVal));
			else
				printf("-Add test suite '%s' to running list Success.\n", pCurSuite->pName);
			pCurSuite = pCurSuite->pNext;
		}
	}
	return RetVal;
	
}

QaError push_case_to_result_list_by_ids(char *case_list, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	char **case_array=NULL;
	char **id_array=NULL;
	int max_array_length=10;
	TestModule *pModule=NULL;
	TestSuite  *pSuite=NULL;
	TestCase   *pCase=NULL;
	int i;

	QA_PRINT(3, ("push_case_to_result_list_by_ids\n"));
	QA_PRINT(3, ("The case list: %s\n", case_list));

	/*1.1.1,2,3.2,4.3.1*/
	if(case_list != NULL){
		case_array = g_strsplit(case_list, ",", max_array_length);
		assert(case_array!=NULL);
		QA_PRINT(3, ("case array size is %d, The first case id is %s\n", g_strv_length(case_array), case_array[0]));
	}

	for(i=0; i<g_strv_length(case_array); i++){
		QA_PRINT(3, ("Parse '%s'.\n", case_array[i]));
		if(case_array[i]!=NULL && g_ascii_isdigit(case_array[i][0])){
			id_array = g_strsplit(case_array[i], ".", 3);
			assert(id_array!=NULL&&id_array[0]!=NULL);
			pModule = qa_frame_find_module_by_ID((int)(g_ascii_strtod(id_array[0],NULL)), &RetVal);
			assert(pModule!=NULL);
			if(id_array[1]!=NULL){
				pSuite = qa_frame_find_suite_by_ID(pModule, (int)(g_ascii_strtod(id_array[1],NULL)), &RetVal);
				assert(pSuite!=NULL);
				if(id_array[2]!=NULL){
					pCase = qa_frame_find_case_by_ID(pSuite, (int)(g_ascii_strtod(id_array[2],NULL)), &RetVal);
					assert(pCase!=NULL);
				}				
			}

			if(pCase!=NULL){
				if(pCase->iCaseWight&priority){
					RetVal = push_case_to_result_list(pModule, pSuite, pCase, g_pFrameStatus);
					if(RetVal != FRAME_SUCCESS)
						printf("---Add test case '%s' to running list faile. ErrorCode='%s'.\n", pCase->pName, qa_frame_get_error_str(RetVal));
					else
						printf("---Add test case '%s' to running list Success.\n", pCase->pName);
				}else
					printf("***%s's priority=%d. ignored.\n", pCase->pName, pCase->iCaseWight);
			}
			else if(pSuite!=NULL)
				push_suite_to_result_list(pModule, pSuite, g_pFrameStatus, priority);
			else
				push_module_to_result_list(pModule, g_pFrameStatus, priority);
		
			pModule=NULL;
			pSuite=NULL;
			pCase=NULL;
			g_strfreev(id_array);
		}
	}

	g_strfreev(case_array);
	
	return RetVal;
}

void send_test_result(gchar *module_name, gchar *suite_name, gchar *case_name,
						CaseResult result, QaError ErrorCode, gint write_pipe)
{
	PipeData data;

	QA_PRINT(3, ("send_test_result(%s,%s,%s,%d,%d,%d)\n",
		module_name, suite_name, case_name, result, ErrorCode, write_pipe));

	strcpy(data.module_name, module_name);
	strcpy(data.suite_name, suite_name);
	strcpy(data.case_name, case_name);

	data.iResult = result;
	data.iFrameError = ErrorCode;
	
    lockf(write_pipe, 1, 0);
    write(write_pipe, &data, sizeof(PipeData));
    lockf(write_pipe, 0, 0);

}

gint read_test_result(PipeData *data, gint read_pipe)
{
	gint len=0;
	
	len = read(read_pipe,data,sizeof(PipeData));
	QA_PRINT(3, ("read from pipe return %d.\n", len));
	if(len==sizeof(PipeData)){
		QA_PRINT(3, ("The data: module_name:%s, suite_name:%s, case_name:%s, iResult:%d, iFrameError:%d\n", 
			data->module_name,data->suite_name,data->case_name, data->iResult, data->iFrameError));
		return TRUE;
	}else if(len==0){
		QA_PRINT(3, ("Read from pipe return 0.\n"));
		return FALSE;
	}else{
		printf("FATAL FAIL - Read from pipe return %d.\n", len);
		exit(1);
	}
}

void set_test_result(TestResult *pTestResult, CaseResult result, QaError ErrorCode)
{
	if(pTestResult == NULL){
		printf("Fatal error [%s:%d]: The pTestResult is null when set test result.\n", __FILE__, __LINE__);
		return;
	}

	QA_PRINT(3, ("set_test_result(%s, %d, %d)\n", 
				pTestResult->pTestCase->pName,
				result,
				ErrorCode));
	
	switch(result)
	{
	case RET_PASS:
		pTestResult->uiTimesOfPass++;
		break;
	case RET_FAIL:
		pTestResult->uiTimesOfFail++;
		pTestResult->ExceptionCode = ErrorCode;
		break;
	case RET_NO_RUN:
		pTestResult->uiTimesOfNoRan++;
		pTestResult->ExceptionCode = ErrorCode;
		break;
	default:
		pTestResult->uiTimesOfCrash++;
		pTestResult->ExceptionCode = ErrorCode;
		break;
	}
}

TestResult* find_test_result(gchar *module_name, gchar *suite_name, gchar *case_name)
{
	TestResult *pCurresult;

	QA_PRINT(3, ("find_test_result().\n"));

	pCurresult = g_pFrameStatus->pTestResult;
	while(pCurresult != NULL){
		if(strcmp(pCurresult->pTestCase->pName, case_name)==0
			&&strcmp(pCurresult->pTestSuite->pName, suite_name)==0
			&&strcmp(pCurresult->pTestModule->pName, module_name)==0){
			QA_PRINT(3, ("Test result item is founded. case_name:%s.\n", pCurresult->pTestCase->pName));
			return pCurresult;
		}
		pCurresult = pCurresult->pNext;
	}
	
	return NULL;
}

QaError do_test_suite_init(TestSuite *pTestSuite)
{
	QaError RetVal = FRAME_FAILURE;
	CaseResult result;
	
	if(pTestSuite==NULL)
		RetVal = FRAME_NO_SUITE;
	else if(pTestSuite->pInitFunc == NULL)
		RetVal = FRAME_SUCCESS;
	else{
		result = (*pTestSuite->pInitFunc)();
		if(result != RET_PASS)
			RetVal = FRAME_FAILURE;
		else
			RetVal = FRAME_SUCCESS;
	}

	return RetVal;
}

CaseResult run_single_case(TestResult *pTestResult, QaError *error)
{
	TestCase *pCase;
	CaseResult result;
	gchar time_str[256];
	time_t now;

	pCase = pTestResult->pTestCase;
	if(pCase->pTestFunc!=NULL){
		now = time(NULL);
		qa_time_to_gmt_str(time_str, &now);
		printf("\n++++++++ Run case(%s-%s-%s) begin (current time:%s)+++++++\n", 
			pTestResult->pTestModule->pName, pTestResult->pTestSuite->pName, pCase->pName, time_str);
			
		result = (*pCase->pTestFunc)();
		*error = FRAME_SUCCESS;
		now = time(NULL);
		qa_time_to_gmt_str(time_str, &now);
		printf("\n++++++++ End (current time:%s)  +++++++\n\n", time_str);		
	}else{
		printf("\n++++++++ No found test function +++++++\n\n");
		result = RET_NO_RUN;
		*error = FRAME_NO_CASE_FUNC;
	}

	return result;
}

void do_test_suite_cleanup(TestSuite *pTestSuite)
{
	QA_PRINT(3, ("do_test_suite_cleanup()\n"));
	
	if(pTestSuite==NULL)
		return;
	else if(pTestSuite->pCleanupFunc == NULL)
		return;
	else{
		(*pTestSuite->pCleanupFunc)();
	}
}

QaError qa_frame_print_out_test_result(void)
{
	TestResult	*pCurResult = NULL;
	TestModule	*pCurModule = NULL;
	TestSuite	*pCurSuite = NULL;

	if(g_pFrameStatus==NULL)
		return FRAME_NO_FRAME_STATUS;
	
	pCurResult = g_pFrameStatus->pTestResult;
	printf("\n>>>>>>Test result summary table.\n");

	if(g_pFrameStatus->iRMode == RUNNING_SEQUENCE){
		gchar tmp[128]; 
		printf("                   Case name              Result   Comments\n");
		while(pCurResult != NULL){
			sprintf(tmp, "(%d.%d.%d)%s", pCurResult->pTestModule->uiID, pCurResult->pTestSuite->uiID, pCurResult->pTestCase->uiID,
											pCurResult->pTestCase->pName);
			printf("%-40s  ", tmp);
			if(pCurResult->uiTimesOfFail>0)
				printf("%-8s  ", "FAIL");
			else if(pCurResult->uiTimesOfNoRan>0)
				printf("%-8s  %-40s", "SKIPPED", qa_frame_get_error_str(pCurResult->ExceptionCode));
			else if(pCurResult->uiTimesOfCrash>0)
				printf("%-8s  %-40s", "CRASH", qa_frame_get_error_str(pCurResult->ExceptionCode));
			else{
				if(pCurResult->uiTimesOfPass<=0)
					assert("The pass time should not equal to 0, if fail,noran and crash are all equal to 0.\n");
				printf("%-8s  ", "PASS");
			}
			printf("\n");

			pCurResult = pCurResult->pNext;
		}
		
	}else{
		printf("                   Case name              Total times  Pass times  Failed times  Crash times  Skipped times\n");
		while(pCurResult != NULL){
			if(pCurResult->pTestSuite!=pCurSuite || pCurResult->pTestModule!=pCurModule){
				pCurModule = pCurResult->pTestModule;
				pCurSuite = pCurResult->pTestSuite;
				printf("[%s-%s]:\n", pCurModule->pName, pCurSuite->pName);
			}
			printf("%40s  %11d  %10d  %12d  %11d  %13d\n", 
				pCurResult->pTestCase->pName, (pCurResult->uiTimesOfPass+pCurResult->uiTimesOfFail+pCurResult->uiTimesOfNoRan+pCurResult->uiTimesOfCrash), 
				(pCurResult->uiTimesOfPass), pCurResult->uiTimesOfFail, pCurResult->uiTimesOfCrash, pCurResult->uiTimesOfNoRan);
			pCurResult = pCurResult->pNext;
		}
	}
	
	return FRAME_SUCCESS;
}

FrameStatus* qa_create_new_frame_status(void)
{
	FrameStatus *pFrameStatus = (FrameStatus *)g_slice_alloc0(sizeof(FrameStatus));

	if(pFrameStatus != NULL){
		pFrameStatus->pCurrentResult = NULL;
		pFrameStatus->iNumOfRemainingCases = 0;
		pFrameStatus->pTestResult = NULL;
		pFrameStatus->iNumOfRandomSteps = 0;
		pFrameStatus->iSumOfCaseWeight = 0;
		pFrameStatus->iStatus = STATUS_STOPPED;
		pFrameStatus->iRMode = RUNNING_SEQUENCE;
	}
	return pFrameStatus;
}


/*
*
* Sequence running mode
*
*/
void sequence_goto_next_suite(FrameStatus *pFrameStatus, gint write_pipe)
{
	QaError RetVal = FRAME_FAILURE;
	TestResult *pCurResult = NULL;

	QA_PRINT(3, ("sequence_goto_next_suite().\n"));

	/*Do the test suite init till one suite init success*/
	while((pFrameStatus->pCurrentResult!=NULL) && (RetVal!=FRAME_SUCCESS)){
		RetVal = do_test_suite_init(pFrameStatus->pCurrentResult->pTestSuite);
		QA_PRINT(3, ("do_test_suite_init(%s) return %s.\n", 
			g_pFrameStatus->pCurrentResult->pTestSuite->pName, qa_frame_get_error_str(RetVal)));			
		if(RetVal != FRAME_SUCCESS){
			pCurResult = pFrameStatus->pCurrentResult;
			while((pCurResult != NULL) && (pCurResult->pTestSuite == pFrameStatus->pCurrentResult->pTestSuite)){
				send_test_result(pCurResult->pTestModule->pName, 
							pCurResult->pTestSuite->pName,
							pCurResult->pTestCase->pName,
							RET_NO_RUN, 
							FRAME_INIT_SUITE_FAIL, 
							write_pipe);
				pFrameStatus->iNumOfRemainingCases--;
				if(pFrameStatus->iNumOfRemainingCases <= 0)
					break;
				pCurResult = pCurResult->pNext;
			}
			pFrameStatus->pCurrentResult = pCurResult;
		}

		if(pFrameStatus->iNumOfRemainingCases <= 0)
			break;
	}

	QA_PRINT(3, ("Current test suite: %s, test case:%s.\n", 
		pFrameStatus->pCurrentResult->pTestSuite->pName,
		pFrameStatus->pCurrentResult->pTestCase->pName));
}

/*Child process function*/
void sequence_case_driver(gint write_pipe)
{
	CaseResult result;
	QaError error;
	QA_PRINT(3, ("child_process_case_driver: iNumOfRemainingCases=%d.\n", g_pFrameStatus->iNumOfRemainingCases));

	sequence_goto_next_suite(g_pFrameStatus, write_pipe);

	while(g_pFrameStatus->iNumOfRemainingCases > 0){
		QA_PRINT(3, ("iNumOfRemainingCases=%d.\n", g_pFrameStatus->iNumOfRemainingCases));
		
		result = run_single_case(g_pFrameStatus->pCurrentResult, &error);
		send_test_result(g_pFrameStatus->pCurrentResult->pTestModule->pName, 
					g_pFrameStatus->pCurrentResult->pTestSuite->pName,
					g_pFrameStatus->pCurrentResult->pTestCase->pName,
					result, 
					error,
					write_pipe);
		sleep(g_uiCaseIntervalTime);
		
		(g_pFrameStatus->iNumOfRemainingCases)--;

		if(g_pFrameStatus->pCurrentResult->pNext!=NULL){
			if(g_pFrameStatus->pCurrentResult->pNext->pTestSuite != g_pFrameStatus->pCurrentResult->pTestSuite){
					do_test_suite_cleanup(g_pFrameStatus->pCurrentResult->pTestSuite);
					g_pFrameStatus->pCurrentResult = g_pFrameStatus->pCurrentResult->pNext;
					sequence_goto_next_suite(g_pFrameStatus, write_pipe);
			}else
				g_pFrameStatus->pCurrentResult = g_pFrameStatus->pCurrentResult->pNext;
		}	
	}

	QA_PRINT(3, ("Frame preparing quit.\n"));

	do_test_suite_cleanup(g_pFrameStatus->pCurrentResult->pTestSuite);
}

pid_t fork_child_process(case_driver driver_f)
{
	pid_t pid;

	QA_PRINT(3, ("fork_child_process().\n"));

	if(pipe_fd[0]!=-1){
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		pipe_fd[0] = -1;
		pipe_fd[1] = -1;
	}	

	QA_PRINT(3, ("Create pipe.\n"));
	if(pipe(pipe_fd)<0)
	{
		printf("pipe create error\n");
		exit(1);
	}

    /* fork */         
	switch (pid = fork()){         
	case -1:             
		printf("FRAME ERROR: fork failed\n");
		exit(1);
		break;
	case 0: 				/*Child process*/
		g_type_init();		
		if (!g_thread_supported ()) {        
			g_thread_init (NULL);    
		}
		close(pipe_fd[0]);		/*Close the read header*/
		pipe_fd[0]=-1;
		case_driver_pid = getpid();
		QA_PRINT(3,("case_driver_pid = getpid() = %d.\n", case_driver_pid));
		driver_f(pipe_fd[1]);
		close(pipe_fd[1]);
		pipe_fd[1]=-1;
		exit(0);
		break;
	default: 				/*Parent process*/
		case_driver_pid = pid;
		QA_PRINT(3, ("The case_driver_pid is %d, main process pid is %d.\n", case_driver_pid, getpid()));
		close(pipe_fd[1]);
		pipe_fd[1]=-1;
		break;
	}	

	return pid;

}

void signal_handler(int sig)
{
	gint status, child_val;
	TestResult *pCurResult = NULL;
	PipeData result_data;
	pid_t child_pid;

	/*If case running process crash, and there are remaining case need running, re-start process*/ 
	QA_PRINT(3, ("child_signal_handler().\n"));
	
	if((child_pid=waitpid(case_driver_pid, &status, WNOHANG)) < 0){
		printf("FRAME ERROR: waitpid failed, return value:%d\n", child_pid); 			
		/*exit(1);*/
		return;
	}else{
		QA_PRINT(3, ("waitpid return %d.\n", child_pid));
	}

	if(child_pid!=case_driver_pid){
		printf("FRAME Message: this process is NOT case driver process, just return.\n");
		printf("This process pid=%d, case_driver_pid=%d.\n", child_pid, case_driver_pid);
		return;
	}

	if(WIFEXITED(status)){			 
		child_val = WEXITSTATUS(status);
		printf("FRAME: child is exited normally with status %d\n", child_val);
	}

	if(WIFSIGNALED(status)){
		child_val = WTERMSIG(status);
		printf("case_driver_process is exit by signal(%d).\n", child_val);
	}

	/*Check if all results have been set*/
	while(read_test_result(&result_data, pipe_fd[0])){
		set_test_result(find_test_result(result_data.module_name,result_data.suite_name,result_data.case_name),	
			result_data.iResult, result_data.iFrameError);
		g_pFrameStatus->iNumOfRemainingCases--;

		if(g_pFrameStatus->iNumOfRemainingCases<0){
			printf("FATAL FAIL, the frame error, reamining case number is 0 but the status is no STOP.\n");
			exit(1);
		}		
	}

	if(g_pFrameStatus->iRMode == RUNNING_RANDOM){
		g_pFrameStatus->iStatus=STATUS_STOPPED;
		if(g_pFrameStatus->iNumOfRandomSteps>0){
			printf("FAIL, There is a case crash. See above logs\n");
		}		
		return;
	}

	/*Go to that case crashed*/
	pCurResult = g_pFrameStatus->pTestResult;
	while(pCurResult!=NULL){
		if((pCurResult->uiTimesOfFail
			+pCurResult->uiTimesOfPass
			+pCurResult->uiTimesOfNoRan
			+pCurResult->uiTimesOfCrash)==0)
			break;
		pCurResult = pCurResult->pNext;
	}

	if(pCurResult == NULL){
		QA_PRINT(3, ("All cases have been run.\n"));
		g_pFrameStatus->iStatus = STATUS_STOPPED;
	}else{
		/*The one maybe lead crash.*/
		QA_PRINT(1,("FRAME: this case maybe lead crash. [%s-%s-%s]\n", 
				pCurResult->pTestModule->pName,
				pCurResult->pTestSuite->pName,
				pCurResult->pTestCase->pName));
		set_test_result(pCurResult,RET_CRASH,FRAME_SUCCESS);
		g_pFrameStatus->iNumOfRemainingCases--;

		/*Skip the crash case then running following*/
		if(pCurResult->pNext == NULL){
			g_pFrameStatus->iStatus = STATUS_STOPPED;
		}else{
			g_pFrameStatus->pCurrentResult = pCurResult->pNext;
			QA_PRINT(3, ("[%s:%d]: Fork to run [%s-%s-%s], the remain cases Num:%d\n", __FILE__,__LINE__,
					g_pFrameStatus->pTestResult->pNext->pTestModule->pName,
					g_pFrameStatus->pTestResult->pNext->pTestSuite->pName,
					g_pFrameStatus->pTestResult->pNext->pTestCase->pName,
					g_pFrameStatus->iNumOfRemainingCases));
			        
			fork_child_process(sequence_case_driver);
		}
	}
}

QaError qa_do_sequence_testing(void)
{
	struct sigaction act, saved;
	PipeData result_data;

	if(g_pFrameStatus->pTestResult==NULL || g_pFrameStatus->iNumOfRemainingCases<=0)
		return FRAME_NO_CASE;

	g_pFrameStatus->iStatus = STATUS_RUNNING;
	g_pFrameStatus->iRMode = RUNNING_SEQUENCE;

	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &act, &saved) < 0){
		printf("Frame Error: sigaction failed\n");
		return FRAME_FAILURE;
	}

	pipe_fd[0]=-1;
	pipe_fd[1]=-1;

	fork_child_process(sequence_case_driver);

	while(g_pFrameStatus->iStatus!=STATUS_STOPPED){

		if(read_test_result(&result_data, pipe_fd[0])){
			set_test_result(find_test_result(result_data.module_name,result_data.suite_name,result_data.case_name),	
				result_data.iResult, result_data.iFrameError);
			g_pFrameStatus->iNumOfRemainingCases--;

			if(g_pFrameStatus->iNumOfRemainingCases<0){
				printf("FATAL FAIL, the frame error, reamining case number is 0 but the status is no STOP.\n");
				exit(1);
			}			
		}
		
		sleep(1);
	}
	close(pipe_fd[0]);
	pipe_fd[0]=-1;


	/*set back saved sigaction*/
	if (sigaction(SIGCHLD, &saved, NULL) < 0){
		printf("Frame Error: sigaction save back failed\n");
	}		

	qa_frame_print_out_test_result();

	return FRAME_SUCCESS;
}

QaError qa_frame_run_all_cases(int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pCurModule;

	printf("\n******run all cases.\n");
	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(g_pTestRegistry->uiNumberOfTotalTestCases == 0)
		RetVal = FRAME_NO_CASE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;

			pCurModule = g_pTestRegistry->pTestModule;
			while(pCurModule != NULL){
				RetVal = push_module_to_result_list(pCurModule, g_pFrameStatus, priority);
				if(RetVal != FRAME_SUCCESS)
					printf("Add test module '%s' to running list faile. ErrorCode='%s'.\n", pCurModule->pName, qa_frame_get_error_str(RetVal));
				else
					printf("Add test module '%s' to running list Success.\n", pCurModule->pName);
				pCurModule = pCurModule->pNext;
			}			
			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;
			
			RetVal = qa_do_sequence_testing();
			if(RetVal != FRAME_SUCCESS)
				printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));

		}
	}

	return RetVal;
}

QaError qa_frame_run_test_suite(gchar *ModuleName, gchar *SuiteName, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);
	TestSuite   *pSuite = qa_find_suite(pModule, SuiteName, &RetVal);

	printf("\n======run test suite: '%s'-'%s'.\n", ModuleName, SuiteName);

	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(pModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(pSuite == NULL)
		RetVal = FRAME_NO_SUITE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;
			
			RetVal = push_suite_to_result_list(pModule, pSuite, g_pFrameStatus, priority);
			if(RetVal != FRAME_SUCCESS)
				printf("Add test Suite '%s-%s' to running list faile. ErrorCode='%s'.\n", ModuleName, SuiteName, qa_frame_get_error_str(RetVal));
			else{
				printf("Add test Suite '%s-%s' to running list Success.\n", ModuleName, SuiteName);
				g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;

				QA_PRINT(3, (">>>>>>Do cases testing. the first case:%s\n", g_pFrameStatus->pCurrentResult->pTestCase->pName));

				RetVal = qa_do_sequence_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));				
			}
		}
	}

	return RetVal;
}

QaError qa_frame_run_test_module(gchar *ModuleName, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);

	printf("\n******run test module: '%s'.\n", pModule->pName);

	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(pModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;
			
			RetVal = push_module_to_result_list(pModule, g_pFrameStatus, priority);
			if(RetVal != FRAME_SUCCESS)
				printf("Add test Module '%s' to running list faile. ErrorCode='%s'.\n", ModuleName, qa_frame_get_error_str(RetVal));
			else{
				printf("Add test Module '%s' to running list Success.\n", ModuleName);
				g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;

				QA_PRINT(3, (">>>>>>Do cases testing. the first case:%s\n", g_pFrameStatus->pCurrentResult->pTestCase->pName));

				RetVal = qa_do_sequence_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));				
			}
		}
	}

	return RetVal;	
}

QaError qa_frame_run_test_case(gchar *ModuleName, gchar *SuiteName, gchar *CaseName)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);
	TestSuite   *pSuite = qa_find_suite(pModule, SuiteName, &RetVal);
	TestCase	   *pCase = qa_find_case_in_suite(pSuite, CaseName, &RetVal);

	printf("\n------run test case: '%s'-'%s'-'%s'.\n", ModuleName, SuiteName, CaseName);

	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(pModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(pSuite == NULL)
		RetVal = FRAME_NO_SUITE;
	else if(pCase == NULL)
		RetVal = FRAME_NO_CASE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;
			
			RetVal = push_case_to_result_list(pModule, pSuite, pCase, g_pFrameStatus);
			if(RetVal != FRAME_SUCCESS)
				printf("Add test Suite '%s-%s-%s' to running list faile. ErrorCode='%s'.\n", ModuleName, SuiteName, CaseName, qa_frame_get_error_str(RetVal));
			else{
				printf("Add test Suite '%s-%s-%s' to running list Success.\n", ModuleName, SuiteName, CaseName);
				g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;
				
				QA_PRINT(3, (">>>>>>Do cases testing. the first case:%s\n", g_pFrameStatus->pCurrentResult->pTestCase->pName));

				RetVal = qa_do_sequence_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));				
			}
		}
	}

	return RetVal;	
}

QaError qa_frame_run_case_by_list(char *case_list, int priority)
{
	QaError RetVal = FRAME_SUCCESS;
	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(case_list == NULL)
		RetVal = FRAME_NO_CASE;
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		RetVal = push_case_to_result_list_by_ids(case_list, priority);
		QA_PRINT(3, ("build cases list return %s.\n", qa_frame_get_error_str(RetVal)));

		if(RetVal == FRAME_SUCCESS){
			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;
			QA_PRINT(3, (">>>>>>Do cases testing. the first case:%s\n", g_pFrameStatus->pCurrentResult->pTestCase->pName));
			RetVal = qa_do_sequence_testing();

			if(RetVal != FRAME_SUCCESS)
				printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));			
		}
			
	}
	
	return RetVal;
}

/*
*
* Random running mode
*
*/
void random_goto_next_case(FrameStatus *pFrameStatus)
{
	gint rand_num=0;

	rand_num = rand()%(pFrameStatus->iSumOfCaseWeight);

	QA_PRINT(3, ("rand number=%d, iSumOfCaseWeight=%d.\n", rand_num, pFrameStatus->iSumOfCaseWeight));

	pFrameStatus->pCurrentResult = pFrameStatus->pTestResult;

	while(pFrameStatus->pCurrentResult!=NULL){
		QA_PRINT(3, ("case %s iSubCaseWeight: %d.\n",
			pFrameStatus->pCurrentResult->pTestCase->pName,
			pFrameStatus->pCurrentResult->iSubCaseWeight));
		if(rand_num <= pFrameStatus->pCurrentResult->iSubCaseWeight)
			break;
		pFrameStatus->pCurrentResult = pFrameStatus->pCurrentResult->pNext;
	}

	if(pFrameStatus->pCurrentResult==NULL){
		printf("FAIL-FATAL ERROR. The random select FAIL\n");
		exit(1);
	}
}

void random_case_driver(gint write_pipe)
{
	CaseResult result;
	QaError error;

	while(g_pFrameStatus->iNumOfRandomSteps > 0){
		sleep(g_uiCaseIntervalTime);
		
		random_goto_next_case(g_pFrameStatus);
		printf("Remaining steps:%d.\n", g_pFrameStatus->iNumOfRandomSteps);

		/*Run the case*/
		result = run_single_case(g_pFrameStatus->pCurrentResult,&error);
		send_test_result(g_pFrameStatus->pCurrentResult->pTestModule->pName, 
			g_pFrameStatus->pCurrentResult->pTestSuite->pName,
			g_pFrameStatus->pCurrentResult->pTestCase->pName,
			result, 
			error,
			write_pipe);
		
		(g_pFrameStatus->iNumOfRandomSteps)--;
	}

}


QaError qa_do_random_testing(void)
{
	QaError RetVal = FRAME_SUCCESS;
	struct sigaction act, saved;
	PipeData result_data;

	if(g_pFrameStatus->pTestResult==NULL || g_pFrameStatus->iNumOfRemainingCases<=0)
		return FRAME_NO_CASE;

	g_pFrameStatus->iRMode = RUNNING_RANDOM;
	g_pFrameStatus->iStatus = STATUS_RUNNING;

	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &act, &saved) < 0){
		printf("Frame Error: sigaction failed\n");
		return FRAME_FAILURE;
	}

	pipe_fd[0]=-1;
	pipe_fd[1]=-1;

	fork_child_process(random_case_driver);

	while(g_pFrameStatus->iStatus!=STATUS_STOPPED){

		if(read_test_result(&result_data, pipe_fd[0])){
			set_test_result(find_test_result(result_data.module_name,result_data.suite_name,result_data.case_name),	
				result_data.iResult, result_data.iFrameError);
			g_pFrameStatus->iNumOfRandomSteps--;

			if(g_pFrameStatus->iNumOfRandomSteps<0){
				printf("FATAL FAIL, the frame error, num of random steps is less than 0 but the status is RUNNING.\n");
				exit(1);
			}			
		}
		
		sleep(1);
	}
	close(pipe_fd[0]);
	pipe_fd[0]=-1;


	/*set back saved sigaction*/
	if (sigaction(SIGCHLD, &saved, NULL) < 0){
		printf("Frame Error: sigaction save back failed\n");
	}

	qa_frame_print_out_test_result();

	return RetVal;
}


QaError qa_frame_random_run_all_cases(guint uiMax_steps, guint uiRand_seed)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pCurModule;
	TestSuite *pCurSuite;
	TestResult *pTestResult=NULL;

	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(g_pTestRegistry->uiNumberOfTotalTestCases == 0)
		RetVal = FRAME_NO_CASE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;

			pCurModule = g_pTestRegistry->pTestModule;
			while(pCurModule != NULL){
				pCurSuite = pCurModule->pTestSuite;
				while(pCurSuite != NULL){
					RetVal = do_test_suite_init(pCurSuite);
					if(RetVal == FRAME_SUCCESS){
						printf("push %s-%s into running list.\n", pCurModule->pName, pCurSuite->pName);
						push_suite_to_result_list(pCurModule, pCurSuite, g_pFrameStatus, CASE_WEIGHT_P1|CASE_WEIGHT_P2|CASE_WEIGHT_P3);
					}else
						printf("WARNING: %s-%s init return %s.\n", pCurModule->pName, pCurSuite->pName, qa_frame_get_error_str(RetVal));
					pCurSuite = pCurSuite->pNext;
				}
				pCurModule = pCurModule->pNext;
			}			
			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;

			
			g_pFrameStatus->iNumOfRandomSteps = (gint)uiMax_steps;
			srand(uiRand_seed);
			if(g_pFrameStatus->iNumOfRandomSteps!=0 && g_pFrameStatus->iNumOfRemainingCases!=0){
				QA_PRINT(3, (">>>>>>Random testing.\n"));
				RetVal = qa_do_random_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));
			}else{
				printf("NO CASE or Random steps = 0.\n");
				RetVal = FRAME_NO_CASE;
			}

			/*Do the clean function*/
			pCurSuite = NULL;
			pTestResult = g_pFrameStatus->pTestResult;
			while(pTestResult != NULL){
				if(pCurSuite != pTestResult->pTestSuite){
					do_test_suite_cleanup(pTestResult->pTestSuite);
					pCurSuite = pTestResult->pTestSuite;
				}
				pTestResult = pTestResult->pNext;
			}

		}
	}

	return RetVal;
}


QaError qa_frame_random_run_test_case(gchar *ModuleName, gchar *SuiteName, gchar *CaseName, guint uiMax_steps, guint uiRand_seed)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule 	*pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);
	TestSuite   *pSuite = qa_find_suite(pModule, SuiteName, &RetVal);
	TestCase	*pCase = qa_find_case_in_suite(pSuite, CaseName, &RetVal);
	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(g_pTestRegistry->uiNumberOfTotalTestCases == 0)
		RetVal = FRAME_NO_CASE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;

			RetVal = do_test_suite_init(pSuite);
			if(RetVal == FRAME_SUCCESS){
				printf("push %s-%s into running list.\n", pModule->pName, pSuite->pName);
				push_case_to_result_list(pModule, pSuite, pCase, g_pFrameStatus);
			}
			else
				printf("WARNING: %s-%s init return %s.\n", pModule->pName, pSuite->pName, qa_frame_get_error_str(RetVal));

			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;
			
			g_pFrameStatus->iNumOfRandomSteps = uiMax_steps;
			srand(uiRand_seed);
			if(g_pFrameStatus->iNumOfRandomSteps!=0 && g_pFrameStatus->iNumOfRemainingCases!=0){
				QA_PRINT(3, (">>>>>>Random testing.\n"));
				RetVal = qa_do_random_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));						
			}else{
				printf("NO CASE or Random steps = 0.\n");
				RetVal = FRAME_NO_CASE;
			}

			if(g_pFrameStatus->iNumOfRemainingCases>0)
				do_test_suite_cleanup(pSuite);

		}
	}

	return RetVal;
}


QaError qa_frame_random_run_test_suite(gchar *ModuleName, gchar *SuiteName, guint uiMax_steps, guint uiRand_seed)
{
	QaError RetVal = FRAME_SUCCESS;
	TestModule *pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);
	TestSuite   *pSuite = qa_find_suite(pModule, SuiteName, &RetVal);	
	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(g_pTestRegistry->uiNumberOfTotalTestCases == 0)
		RetVal = FRAME_NO_CASE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;

			RetVal = do_test_suite_init(pSuite);
			if(RetVal == FRAME_SUCCESS){
				printf("push %s-%s into running list.\n", pModule->pName, pSuite->pName);
				push_suite_to_result_list(pModule, pSuite, g_pFrameStatus, CASE_WEIGHT_P1|CASE_WEIGHT_P2|CASE_WEIGHT_P3);
			}
			else
				printf("WARNING: %s-%s init return %s.\n", pModule->pName, pSuite->pName, qa_frame_get_error_str(RetVal));

			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;
			
			g_pFrameStatus->iNumOfRandomSteps = uiMax_steps;
			srand(uiRand_seed);
			if(g_pFrameStatus->iNumOfRandomSteps!=0 && g_pFrameStatus->iNumOfRemainingCases!=0){
				QA_PRINT(3, (">>>>>>Random testing.\n"));
				RetVal = qa_do_random_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));						
			}else{
				printf("NO CASE or Random steps = 0.\n");
				RetVal = FRAME_NO_CASE;
			}

			if(g_pFrameStatus->iNumOfRemainingCases>0)
				do_test_suite_cleanup(pSuite);

		}
	}

	return RetVal;
}

QaError qa_frame_random_run_test_module(gchar *ModuleName, guint uiMax_steps, guint uiRand_seed)
{
	QaError RetVal = FRAME_SUCCESS;
	TestResult *pTestResult=NULL;
	TestSuite *pCurSuite=NULL;
	TestModule *pModule = qa_find_module(g_pTestRegistry, ModuleName, &RetVal);
	
	if(g_pTestRegistry == NULL)
		RetVal = FRAME_NO_REGISTRY;
	else if(g_pTestRegistry->uiNumberOfTotalTestCases == 0)
		RetVal = FRAME_NO_CASE;
	else if(pModule == NULL)
		RetVal = FRAME_NO_MODULE;
	else if(g_pFrameStatus!=NULL && g_pFrameStatus->iStatus!=STATUS_STOPPED)
		RetVal = FRAME_ALREADY_START;	
	else{
		QA_PRINT(3, (">>>>>>Clear previous results.\n"));
		qa_clear_previous_results();
		if(g_pFrameStatus == NULL)
			g_pFrameStatus = qa_create_new_frame_status();

		QA_PRINT(3, (">>>>>>Build cases running list.\n"));
		if(g_pFrameStatus == NULL)
			RetVal = FRAME_NO_MEMORY;
		else{
			g_pFrameStatus->iNumOfRemainingCases = 0;
			g_pFrameStatus->iSumOfCaseWeight = 0;

			pCurSuite = pModule->pTestSuite;
			while(pCurSuite != NULL){
				RetVal = do_test_suite_init(pCurSuite);
				if(RetVal == FRAME_SUCCESS){
					printf("push %s-%s into running list.\n", pModule->pName, pCurSuite->pName);
					push_suite_to_result_list(pModule, pCurSuite, g_pFrameStatus, CASE_WEIGHT_P1|CASE_WEIGHT_P2|CASE_WEIGHT_P3);
				}
				else
					printf("WARNING: %s-%s init return %s.\n", pModule->pName, pCurSuite->pName, qa_frame_get_error_str(RetVal));
				pCurSuite = pCurSuite->pNext;
			}
		
			g_pFrameStatus->pCurrentResult = g_pFrameStatus->pTestResult;

			g_pFrameStatus->iNumOfRandomSteps = uiMax_steps;
			srand(uiRand_seed);
			if(g_pFrameStatus->iNumOfRandomSteps!=0 && g_pFrameStatus->iNumOfRemainingCases!=0){
				printf(">>>>>>Random testing.\n");
				RetVal = qa_do_random_testing();
				if(RetVal != FRAME_SUCCESS)
					printf("Running cases error. error='%s'.\n", qa_frame_get_error_str(RetVal));
			}else{
				printf("NO CASE or Random steps = 0.\n");
				RetVal = FRAME_NO_CASE;
			}

			/*Do the clean function*/
			pCurSuite = NULL;
			pTestResult = g_pFrameStatus->pTestResult;
			while(pTestResult != NULL){
				if(pCurSuite != pTestResult->pTestSuite){
					do_test_suite_cleanup(pTestResult->pTestSuite);
					pCurSuite = pTestResult->pTestSuite;
				}
				pTestResult = pTestResult->pNext;
			}


		}
	}

	return RetVal;
}


QaError qa_frame_case_set_weight(TestSuite *pTestSuite, const gchar *pCaseName, gint weight)
{
	TestCase *pTestCase;
	QaError error;

	if(pTestSuite==NULL)
		return FRAME_NO_SUITE;
	else if(pCaseName==NULL)
		return FRAME_NO_CASE;
	else if(weight<0)
		return FRAME_FAILURE;
	else if(g_pTestRegistry==NULL){
		QA_PRINT(3, ("FAIL - FATAL ERROR, no REGISTRY.\n"));
		return FRAME_NO_REGISTRY;
	}else{
		pTestCase = qa_find_case_in_suite(pTestSuite, pCaseName, &error);
		if(error!=FRAME_SUCCESS)
			return error;

		pTestCase->iCaseWight = weight;
		return FRAME_SUCCESS;
	}
}

QaError qa_frame_case_interval_time_set(guint seconds)
{
	g_uiCaseIntervalTime = seconds;
	return FRAME_SUCCESS;
}

QaError qa_frame_case_interval_time_get(guint *seconds)
{
	*seconds = g_uiCaseIntervalTime;
	return FRAME_SUCCESS;
}

TestCase* qa_frame_get_current_case(QaError *ErrorCode)
{
	if(g_pFrameStatus==NULL){
		*ErrorCode = FRAME_NO_FRAME_STATUS;
		return NULL;
	}else if(g_pFrameStatus->pCurrentResult==NULL){
		*ErrorCode = FRAME_FAILURE;
		return NULL;
	}else{
		*ErrorCode = FRAME_SUCCESS;
		return g_pFrameStatus->pCurrentResult->pTestCase;
	}
}

TestSuite* qa_frame_get_current_suite(QaError *ErrorCode)
{
	if(g_pFrameStatus==NULL){
		*ErrorCode = FRAME_NO_FRAME_STATUS;
		return NULL;
	}else if(g_pFrameStatus->pCurrentResult==NULL){
		*ErrorCode = FRAME_FAILURE;
		return NULL;
	}else{
		*ErrorCode = FRAME_SUCCESS;
		return g_pFrameStatus->pCurrentResult->pTestSuite;
	}

}

TestModule* qa_frame_get_current_module(QaError *ErrorCode)
{
	if(g_pFrameStatus==NULL){
		*ErrorCode = FRAME_NO_FRAME_STATUS;
		return NULL;
	}else if(g_pFrameStatus->pCurrentResult==NULL){
		*ErrorCode = FRAME_FAILURE;
		return NULL;
	}else{
		*ErrorCode = FRAME_SUCCESS;
		return g_pFrameStatus->pCurrentResult->pTestModule;
	}

}

/*Print Case list*/
QaError qa_frame_print_case_info(TestModule *pModule, TestSuite *pSuite, TestCase *pCase)
{
	if(pModule==NULL)
		return FRAME_NO_MODULE;
	else if(pSuite==NULL)
		return FRAME_NO_SUITE;
	else if(pCase==NULL)
		return FRAME_NO_CASE;
	else{
		printf("\t\t (%s) %d.%d.%d - %s\n",
			pCase->iCaseWight>=CASE_WEIGHT_P1?"P1":(pCase->iCaseWight>=CASE_WEIGHT_P2?"P2":"P3"),
			pModule->uiID, pSuite->uiID, pCase->uiID, pCase->pName);

		return FRAME_SUCCESS;
	}
}

QaError qa_frame_print_suite_info(TestModule *pModule, TestSuite *pSuite)
{
	if(pModule==NULL)
		return FRAME_NO_MODULE;
	else if(pSuite==NULL)
		return FRAME_NO_SUITE;
	else{
		TestCase *pCase;

		printf("\t\t%d.%d - %s\n", pModule->uiID, pSuite->uiID, pSuite->pName);
		
		pCase = pSuite->pTestCase;
		while(pCase!=NULL){
			qa_frame_print_case_info(pModule, pSuite, pCase);
			pCase = pCase->pNext;
		}

		return FRAME_SUCCESS;
	}
}

QaError qa_frame_print_module_info(TestModule *pModule)
{
	if(pModule==NULL)
		return FRAME_NO_MODULE;
	else{
		TestSuite *pSuite;

		printf("\t%d - %s\n", pModule->uiID, pModule->pName);

		pSuite = pModule->pTestSuite;

		while(pSuite!=NULL){
			qa_frame_print_suite_info(pModule, pSuite);
			pSuite = pSuite->pNext;
		}

		return FRAME_SUCCESS;
	}
}

QaError qa_frame_print_case_list(void)
{
	if(g_pTestRegistry==NULL)
		return FRAME_NO_REGISTRY;
	else{
		TestModule *pModule;
		printf("----------------------\n");

		pModule = g_pTestRegistry->pTestModule;
		while(pModule!=NULL){
			qa_frame_print_module_info(pModule);
			pModule = pModule->pNext;
		}

		printf("----------------------\n");

		return FRAME_SUCCESS;
	}
}

QaError qa_frame_print_case_list_by_ID(char *case_list)
{
	QaError RetVal = FRAME_SUCCESS;
	char **case_array=NULL;
	char **id_array=NULL;
	int max_array_length=10;
	TestModule *pModule=NULL;
	TestSuite  *pSuite=NULL;
	TestCase   *pCase=NULL;
	int i;

	QA_PRINT(3, ("qa_frame_print_case_list_by_ID\n"));
	QA_PRINT(3, ("The case list: %s\n", case_list));

	/*1.1.1,2,3.2,4.3.1*/
	if(case_list != NULL){
		case_array = g_strsplit(case_list, ",", max_array_length);
		assert(case_array!=NULL);
		QA_PRINT(3, ("case array size is %d, The first case id is %s\n", g_strv_length(case_array), case_array[0]));
	}

	printf("----------------------\n");

	for(i=0; i<g_strv_length(case_array); i++){
		QA_PRINT(3, ("Parse '%s'.\n", case_array[i]));
		if(case_array[i]!=NULL && g_ascii_isdigit(case_array[i][0])){
			id_array = g_strsplit(case_array[i], ".", 3);
			assert(id_array!=NULL&&id_array[0]!=NULL);
			pModule = qa_frame_find_module_by_ID((int)(g_ascii_strtod(id_array[0],NULL)), &RetVal);
			assert(pModule!=NULL);
			if(id_array[1]!=NULL){
				pSuite = qa_frame_find_suite_by_ID(pModule, (int)(g_ascii_strtod(id_array[1],NULL)), &RetVal);
				assert(pSuite!=NULL);
				if(id_array[2]!=NULL){
					pCase = qa_frame_find_case_by_ID(pSuite, (int)(g_ascii_strtod(id_array[2],NULL)), &RetVal);
					assert(pCase!=NULL);
				}				
			}

			if(pCase!=NULL)
				qa_frame_print_case_info(pModule, pSuite, pCase);
			else if(pSuite!=NULL)
				qa_frame_print_suite_info(pModule, pSuite);
			else
				qa_frame_print_module_info(pModule);
		
			pModule=NULL;
			pSuite=NULL;
			pCase=NULL;
			g_strfreev(id_array);
		}
	}

	printf("----------------------\n");

	g_strfreev(case_array);
	
	return RetVal;

}

TestModule *qa_frame_find_module_by_ID(guint module_id, QaError *error_code)
{
	QA_PRINT(3, ("qa_frame_find_module_by_ID, module_id=%d\n", module_id));

	if(g_pTestRegistry==NULL){
		*error_code = FRAME_NO_REGISTRY;
		return NULL;
	}else{
		TestModule *pModule;

		error_code = FRAME_SUCCESS;
		pModule = g_pTestRegistry->pTestModule;
		while(pModule!=NULL){
			if(pModule->uiID == module_id)
				return pModule;
			pModule = pModule->pNext;
		}

		return NULL;
	}

}

TestSuite *qa_frame_find_suite_by_ID(TestModule *pModule, guint suite_id, QaError *error_code)
{
	QA_PRINT(3, ("qa_frame_find_suite_by_ID, suite_id=%d\n", suite_id));
	
	if(pModule==NULL){
		*error_code = FRAME_NO_MODULE;
		return NULL;
	}else if(pModule->pTestSuite==NULL){
		*error_code = FRAME_NO_SUITE;
		return NULL;
	}else{
		TestSuite *pSuite=pModule->pTestSuite;

		error_code = FRAME_SUCCESS;
		while(pSuite!=NULL){
			if(pSuite->uiID == suite_id)
				return pSuite;
			pSuite = pSuite->pNext;
		}

		return NULL;
	}
}

TestCase *qa_frame_find_case_by_ID(TestSuite *pSuite, guint case_id, QaError *error_code)
{
	QA_PRINT(3, ("qa_frame_find_case_by_ID, case_id=%d\n", case_id));

	if(pSuite==NULL){
		*error_code = FRAME_NO_SUITE;
		return NULL;
	}else if(pSuite->pTestCase==NULL){
		*error_code = FRAME_NO_CASE;
		return NULL;
	}else{
		TestCase *pCase=pSuite->pTestCase;

		*error_code = FRAME_SUCCESS;
		while(pCase!=NULL){
			if(pCase->uiID==case_id)
				return pCase;
			pCase = pCase->pNext;
		}

		return NULL;
		
	}
}

