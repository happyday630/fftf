/*----------------------------------------------------------
 | Quality Test suite
 | Copyright (C) Opentv
 | ----------------------------------------------------------
 | QA test frame which managing tests and control cases running.
 |
 | File Name: 		frame.h
 | Author:		Hanson
 | Created date: 	11/03/2008
 | Version: 		v_0_1
 | Last update: 	09/23/2011 Modify for OPENTV5 MCA
 | Updated by: 	Hanson
 +----------------------------------------------------------*/
 
#ifndef __FRAME_H__
#define __FRAME_H__

/*----------------------------------------------------------
 | Include definition below this line
 +----------------------------------------------------------*/
#include <stdlib.h>
#include <glib.h>

/*----------------------------------------------------------
 | Macro definition below this line
 +----------------------------------------------------------*/
#define QA_TEST_DEFAULT_PRINT_LEVEL	1
#define QASTMT( stuff )		do { stuff } while( 0 )
#define QA_PRINT( lvl, txt )	QASTMT( if ( QA_TEST_DEFAULT_PRINT_LEVEL >= lvl ) {printf("COMMENT-[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); printf txt ;} )

#define QA_PRINT_PASS( txt )	QASTMT( printf("PASS-[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); printf txt; printf("[test_bed][MCA_TS][Stop][0][end]\n");)
#define QA_PRINT_FAIL( txt )    QASTMT( printf("FAIL-[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); printf txt; printf("[test_bed][MCA_TS][Stop][-1][end]\n");)
/*----------------------------------------------------------
 | Type definition below this line
 +----------------------------------------------------------*/
typedef enum
{
	RET_PASS,
	RET_FAIL,
	RET_NO_RUN,
	RET_CRASH,
}CaseResult;

typedef enum
{
	FRAME_SUCCESS = 0,
	FRAME_FAILURE = -1,
	FRAME_NO_MEMORY = -2,
	FRAME_NO_MODULE_NAME = -3,
	FRAME_NO_REGISTRY = -4,
	FRAME_DUPLICATE_MODULE = -5,
	FRAME_NO_MODULE = -6,
	FRAME_NO_SUITE_NAME = -7,
	FRAME_NO_SUITE = -8,
	FRAME_DUPLICATE_SUITE = -9,
	FRAME_NO_CASE_NAME = -10,
	FRAME_NO_CASE = -11,
	FRAME_DUPLICATE_CASE = -12,
	FRAME_NO_CASE_FUNC = -13,
	FRAME_INIT_SUITE_FAIL = -14,
	FRAME_NO_FRAME_STATUS = -15,
	FRAME_TIMER_CREATE_ERROR = -16,
	FRAME_TIMER_START_ERROR = -17,
	FRAME_NO_VERIF_FUNC = -18,
	FRAME_NO_RESULT_RECORD = -19,
	FRAME_ALREADY_START = -20,
}QaError;

typedef enum
{
	RUNNING_SEQUENCE = 1,
	RUNNING_RANDOM	= 2,
}RunningMode;

typedef enum
{
	CASE_WEIGHT_LOWEST = 16,
	CASE_WEIGHT_LOW = 32,
	CASE_WEIGHT_P3 = 32,
	CASE_WEIGHT_MEDIUM = 64,
	CASE_WEIGHT_P2 = 64,
	CASE_WEIGHT_HIGH = 128,
	CASE_WEIGHT_P1 = 128,
	CASE_WEIGHT_HIGHEST = 256,
}CaseWight;

/*Case manage structs*/

typedef CaseResult (*TestFunc)(void);
typedef CaseResult (*InitFunc)(void);
typedef void (*CleanupFunc)(void);

typedef struct TestCase
{
	gchar 		*pName;					/*case name*/
	TestFunc		pTestFunc;			/*pointer to test function*/
	gint			iCaseWight;			/*wight value of case*/
	guint			uiID;				/*Case number auto assigned by framework*/
	struct TestCase		*pPrev;			/*pointer to previous case in list*/
	struct TestCase		*pNext;			/*pointer to next case in list*/
}TestCase;

typedef struct TestSuite
{
	gchar 		*pName;				/*test suite name*/
	struct TestCase	*pTestCase;		/*pointer to first test in the test suite*/
	InitFunc		pInitFunc;		/*pointer to init function of test suite*/
	CleanupFunc	pCleanupFunc;		/*pointer to cleanup function*/
	guint	uiNumberOfTest;			/*Total tests of test suite*/
	guint		uiID;				/*Suite number auto assigned by framework*/
	struct TestSuite	*pPrev;		/*pointer to previous test suite in list*/
	struct TestSuite	*pNext;		/*pointer to next test suite in list*/
}TestSuite;

typedef struct TestModule
{
	gchar			*pName;				/*test module name*/
	struct TestSuite	*pTestSuite;	/*pointer to first test suite*/
	guint			uiNumberOfSuite;	/*Total test suite of test Module*/
	guint			uiID;				/*Module ID auto assigned by framework*/
	struct TestModule	*pPrev;			/*pointer to previous test module in list*/
	struct TestModule	*pNext;			/*pointer to next test module in list*/
}TestModule;

typedef struct TestRegistry
{
	guint	uiNumberOfTotalTestModules;				/*Number of total test modules*/
	guint 	uiNumberOfTotalTestSuites;				/*Number of total test suites*/
	guint	uiNumberOfTotalTestCases;				/*Number of total test cases*/
	struct TestModule	*pTestModule;				/*pointer to first test module*/
}TestRegistry;


/*----------------------------------------------------------
 | Global vars definition below this line
 +----------------------------------------------------------*/


/*----------------------------------------------------------
 | Case Managing Functions definition below this line
 +----------------------------------------------------------*/
/*
 parameter:
 	N/A
 return value:
 	If success, it will return FRAME_SUCCESS. otherwise, it will return error code. Please see QaError for more detail.
 Description:
 	This function initialize whole test frame, create a test registry. 
 	This function should be called before add any test modules, suites or cases.
 */
extern QaError qa_frame_initialize_test_registry();

 /*
 parameter:
 	N/A
 return value:
 	N/A
 Description:
 	This function cleanup whole test frame, delete all added modules, suites and cases. Free all allocated memory. 
 	It should be called before application exit.
 */
extern void qa_frame_cleanup_registry();

 /*
 parameter:
 	pName 		- The name of test module
 	ErrorCode 	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid TestModule pointer if add test module success, otherwise return NULL.
 Description:
 	This function create a new module then insert to test frame.
 */
extern TestModule* qa_frame_add_test_module(const gchar* pName, QaError *ErrorCode);

 /*
 parameter:
 	pName		- The Name of test module
 	ErrorCode 	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid TestModule pointer if it exist, otherwise return NULL.
 Description:
 	This function search whole test frame to find desired test module.
 */
extern TestModule* qa_frame_get_module_by_name(const gchar *pName, QaError *ErrorCode);

 /*
 parameter:
		ErrorCode 	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 		It will return a valid TestModule pointer if it exist, otherwise return NULL.
 Description:
 	This function the first test module.
 */
extern TestModule* qa_frame_get_first_module(QaError *ErrorCode);

/*
 parameter:
 	pModule 		- Pointer to parent test module of test suite
 	pSuiteName	- The name of test suite.
 	pInitFunc		- The init function of whole test suite. NULL for no init function.
 	pCleanupFunc	- The cleanup function of whole test suite. NULL for no cleanup function.
 	ErrorCode	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid test suite pointer if add test suite success, otherwise return NULL.
 Description:
 	This function create a new test suite, then insert it to specified test module.
 */ 
extern TestSuite* qa_frame_add_test_suite(TestModule *pModule, const gchar* pSuiteName, InitFunc pInitFunc, CleanupFunc pCleanupFunc, QaError *ErrorCode);

 /*
 parameter:
 	pTestModule	- The parent test module
 	pSuiteName	- The Name of test suite
 	ErrorCode 	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid Test suite pointer if it exist, otherwise return NULL.
 Description:
 	This function search parent test module to find desired test suite.
 */
extern TestSuite* qa_frame_get_suite_by_name(TestModule *pTestModule, const gchar *pSuiteName, QaError *ErrorCode);

 /*
 parameter:
 	pTestSuite 	- Pointer to parent test suite of test case
 	pCaseName	- The name of test case.
 	pTestFunc	- The function of this case, should not be NULL.
 	ErrorCode	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid test case pointer if add test case success, otherwise return NULL.
 Description:
 	This function create a new test case, then insert it to specified test suite.
 */ 
extern TestCase* qa_frame_add_test_case(TestSuite *pTestSuite, const gchar *pCaseName, TestFunc pTestFunc, QaError *ErrorCode);

 /*
 parameter:
 	pCaseName	- The Name of test case
 	ErrorCode 	- This is output parameter. It will return FRAME_SUCCESS if success. 
 				otherwise, it return error code. please see QaError for more detail
 return value:
 	It will return a valid Test case pointer if it exist, otherwise return NULL.
 Description:
 	This function search test case by name in whole pool.
 */
extern TestCase* qa_frame_get_case_by_name(const gchar *pCaseName, QaError *ErrorCode);

 /*
 parameter:
 	N/A
 return value:
 	The number of total test modules in test frame.
 Description:
 	This function get the total test modules current test frame have.
 */  
extern guint qa_frame_get_total_modules();

 /*
 parameter:
 	N/A
 return value:
 	The number of total test suites in test frame.
 Description:
 	This function get the total test suites current test frame have.
 */   
extern guint qa_frame_get_total_suites();

 /*
 parameter:
 	N/A
 return value:
 	The number of total test cases in test frame.
 Description:
 	This function get the total test cases current test frame have.
 */   
extern guint qa_frame_get_total_cases();

 /*
 parameter:
 	ErrorCode	- The error code
 return value:
 	A string of error code's description
 Description:
 	This function return the description of error code.
 */   
extern const gchar *qa_frame_get_error_str(QaError ErrorCode);


/*----------------------------------------------------------
 | Case control running Functions definition below this line
 +----------------------------------------------------------*/
 /*
 parameter:
 	priority		-	A filter to tell case driver which type cases you want to run, such as P1, P2 or P3.
 return value:
 	SUCCESS or Error Code
 Description:
 	This function run all cases added to frame. In the last it'll output the summary table.
 */  
extern QaError qa_frame_run_all_cases(int priority);

 /*
 parameter:
 	ModuleName	- The name of the module
    	priority		- A filter to tell case driver which type cases you want to run, such as p1, p2 or p3.	
 return value:
 	SUCCESS or Error Code
 Description:
 	This function run all cases under specified test module. In the last it'll output the summary table.
 */ 
extern QaError qa_frame_run_test_module(gchar *ModuleName, int priority);

 /*
 parameter:
 	ModuleName	- The name of test module
 	SuiteName	- The name of test suite
    	priority		- A filter to tell case driver which type cases you want to run, such as p1, p2 or p3.	
 return value:
 	SUCCESS or Error Code
 Description:
 	This function run all cases under specified test suite. In the last it'll output the summary table.
 */  
extern QaError qa_frame_run_test_suite(gchar *ModuleName, gchar *SuiteName, int priority);

 /*
 parameter:
 	ModuleName	- The name of test module
 	SuiteName	- The name of test suite
 	CaseName	- The name of test case
 return value:
 	SUCCESS or Error Code
 Description:
 	This function run specified case. In the last it'll output the summary table.
 */ 
extern QaError qa_frame_run_test_case(gchar *ModuleName, gchar *SuiteName, gchar *CaseName);

/*
parameter:
   case_list	-	A string of case id list. like "1.1.2,2.3,3,4.2.1"
   priority		-	A filter to tell case driver which type cases you want to run, such as p1, p2 or p3.
return value:
   SUCCESS or Error Code
Description:
   This function run a list of case. In the last it'll output the summary table.
*/ 
extern QaError qa_frame_run_case_by_list(char *case_list, int priority);

 /*
 parameter:
 	pTestSuite	- Pointer to test suite
	pCaseName	- Pointer to case name
 	weighing		- The new weighing.
 return value:
 	SUCCESS or Error Code
 Description:
 	The case when added to test frame will be set with the default weighing value, CASE_WEIGHING_MEDIUM. You can call this API to change it.
 	The values please reference CaseWight data type.
 */ 
QaError qa_frame_case_set_weight(TestSuite *pTestSuite, const gchar *pCaseName, gint weight);


/*below APIs let you run a group cases in random*/
QaError qa_frame_random_run_all_cases(guint uiMax_steps, guint uiRand_seed);
QaError qa_frame_random_run_test_module(gchar *ModuleName, guint uiMax_steps, guint uiRand_seed);
QaError qa_frame_random_run_test_suite(gchar *ModuleName, gchar *SuiteName, guint uiMax_steps, guint uiRand_seed);
QaError qa_frame_random_run_test_case(gchar *ModuleName, gchar *SuiteName, gchar *CaseName, guint uiMax_steps, guint uiRand_seed);

/*Advanced APIs*/
QaError qa_frame_print_out_test_result();
QaError qa_frame_case_interval_time_set(unsigned int seconds);
QaError qa_frame_case_interval_time_get(unsigned int *seconds);
TestCase* qa_frame_get_current_case(QaError *ErrorCode);
TestSuite* qa_frame_get_current_suite(QaError *ErrorCode);
TestModule* qa_frame_get_current_module(QaError *ErrorCode);

/*Case list query APIs*/
QaError qa_frame_print_case_list(void);
QaError qa_frame_print_case_list_by_ID(char *case_list);
QaError qa_frame_print_case_info(TestModule * pModule,TestSuite * pSuite,TestCase * pCase);
QaError qa_frame_print_suite_info(TestModule * pModule,TestSuite * pSuite);
QaError qa_frame_print_module_info(TestModule * pModule);


/*Find case by ID APIs*/
TestCase *qa_frame_find_case_by_ID(TestSuite *pSuite, guint case_id, QaError *error_code);
TestSuite *qa_frame_find_suite_by_ID(TestModule *pModule, guint suite_id, QaError *error_code);
TestModule *qa_frame_find_module_by_ID(guint module_id, QaError * error_code);



/*utilities function*/
void qa_time_to_gmt_str(gchar *str_gmt_time, time_t *time_in);

#endif /*__FRAME_H__*/

