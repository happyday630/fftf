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
#include <unistd.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "../frame.h"
#include "test_cases.h"
#include "../test_common.h"

/*----------------------------------------------------------
 | Macro definition below this line
 +----------------------------------------------------------*/


/*----------------------------------------------------------
 | Type definition below this line
 +----------------------------------------------------------*/


/*----------------------------------------------------------
 | Global vars definition below this line
 +----------------------------------------------------------*/

/*----------------------------------------------------------
 | Functions definition below this line
 +----------------------------------------------------------*/
static CaseResult pass_suite_init(void)
{
	CaseResult RetVal=RET_PASS;
	QA_PRINT_PASS(("The suite init is success.\n"));
	return RetVal;
}

static CaseResult fail_suite_init(void)
{
	CaseResult RetVal=RET_FAIL;
	QA_PRINT_FAIL(("The suite init is failure.\n"));
	return RetVal;

}

static void suite_clean(void)
{
	QA_PRINT(1, ("The suite clean function.\n"));
	return;
}

static CaseResult pass_case(void)
{
	CaseResult RetVal=RET_PASS;
	QA_PRINT_PASS(("Hello world. it is pass case\n"));
	return RetVal;
}

static CaseResult fail_case(void)
{
	CaseResult RetVal=RET_FAIL;
	QA_PRINT_FAIL(("Hello world. it is fail case\n"));
	return RetVal;
}

static CaseResult multiple_process_test(void)
{
	CaseResult RetVal = RET_PASS;
	pid_t pid;

	    /* fork */         
	switch (pid = fork()){         
	case -1:             
		printf("FRAME ERROR: fork failed\n");
		exit(1);
		break;
	case 0: 				/*Child process*/
		QA_PRINT(1, ("the child PID is %d. \n", getpid()));
		exit(0);
		break;
	default: 				/*Parent process*/
		QA_PRINT(1, ("The parent PID is %d\n", pid));
		break;
	}	

	
	
	return RetVal;
}

static CaseResult system_call(void)
{
    CaseResult result = RET_PASS;
    int fd       = -1;
    int temp_ret = -1;

    printf("Enter system_call\n");

    printf("starting call bash script\n");
    temp_ret=system("find /mnt/ -name copenfile* -print");
    printf("system return value: %d\n", temp_ret);

    printf("Quit system_call\n");
    return result;

}

static CaseResult crash_case(void)
{
	TestSuite *pSuite=NULL;
	QA_PRINT(1, ("Crash case.\n"));
	pSuite->pName=NULL;
	return RET_FAIL;
}

/*example for Test module init*/
CaseResult test_module_init(void)
{
	TestModule	*pModule = NULL;
	TestSuite	*pSuite;
	QaError		error;
	CaseResult 	retVal = RET_PASS;

	pModule = qa_frame_add_test_module("test_module", &error);	
	
	if(pModule == NULL){		
		QA_PRINT_FAIL(("Can't add test_module. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add test_module success.error='%s'\n", qa_frame_get_error_str(error)));			

	/*Good test suite*/
	pSuite = qa_frame_add_test_suite(pModule, "good_test_suite", pass_suite_init, suite_clean, &error);	
	if(pSuite == NULL){		
		QA_PRINT_FAIL(("Can't add test suite into frame. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add test suite into module success. error='%s'\n", qa_frame_get_error_str(error)));		

	qa_frame_add_test_case(pSuite,"pass_case",pass_case,&error);
	qa_frame_add_test_case(pSuite,"fail_case",fail_case,&error);
	qa_frame_add_test_case(pSuite, "multiple_process_test", multiple_process_test, &error);
	qa_frame_add_test_case(pSuite, "system_call", system_call, &error);
	qa_frame_add_test_case(pSuite,"pass_case_again",pass_case,&error);
	qa_frame_add_test_case(pSuite,"fail_case_again",fail_case,&error);	

	qa_frame_case_set_weight(pSuite, "multiple_process_test", CASE_WEIGHT_P1);
	qa_frame_case_set_weight(pSuite, "fail_case_again", CASE_WEIGHT_P3);

	/*init_fail_suite*/
	pSuite = qa_frame_add_test_suite(pModule, "init_fail_suite", fail_suite_init, suite_clean, &error);	
	if(pSuite == NULL){		
		QA_PRINT_FAIL(("Can't add test suite into frame. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add test suite into module success. error='%s'\n", qa_frame_get_error_str(error)));		

	qa_frame_add_test_case(pSuite,"init_fail_pass_case",pass_case,&error);
	qa_frame_add_test_case(pSuite,"init_fail_fail_case",fail_case,&error);
	qa_frame_add_test_case(pSuite,"init_fail_pass_case_again",pass_case,&error);
	qa_frame_add_test_case(pSuite,"init_fail_fail_case_again",fail_case,&error);	

	/*crash test suite*/
	pSuite = qa_frame_add_test_suite(pModule, "crash_test_suite", pass_suite_init, suite_clean, &error);	
	if(pSuite == NULL){		
		QA_PRINT_FAIL(("Can't add test suite into frame. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add test suite into module success. error='%s'\n", qa_frame_get_error_str(error)));		

	qa_frame_add_test_case(pSuite,"crash_in_the_first",crash_case,&error);
	qa_frame_add_test_case(pSuite,"pass_case",pass_case,&error);
	qa_frame_add_test_case(pSuite,"crash_in_the_middle",crash_case,&error);
	qa_frame_add_test_case(pSuite,"fail_case",fail_case,&error);
	qa_frame_add_test_case(pSuite,"crash_in_the_end",crash_case,&error);

	qa_frame_case_set_weight(pSuite, "crash_in_the_middle", CASE_WEIGHT_P1);
	qa_frame_case_set_weight(pSuite, "fail_case", CASE_WEIGHT_P3);



	/*Second test module*/
	pModule = qa_frame_add_test_module("second_module", &error);	
	
	if(pModule == NULL){		
		QA_PRINT_FAIL(("Can't add second_module. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add second_module success.error='%s'\n", qa_frame_get_error_str(error)));			

	/*Good test suite*/
	pSuite = qa_frame_add_test_suite(pModule, "good_test_suite", NULL, NULL, &error);	
	if(pSuite == NULL){		
		QA_PRINT_FAIL(("Can't add test suite into frame. error='%s'\n", qa_frame_get_error_str(error)));
		retVal = RET_FAIL;
	}else		
		QA_PRINT_PASS(("Add test suite into module success. error='%s'\n", qa_frame_get_error_str(error)));		

	qa_frame_add_test_case(pSuite,"pass_case",pass_case,&error);
	qa_frame_add_test_case(pSuite,"fail_case",fail_case,&error);
	qa_frame_add_test_case(pSuite,"pass_case_again",pass_case,&error);
	qa_frame_add_test_case(pSuite,"fail_case_again",fail_case,&error);	

	qa_frame_case_set_weight(pSuite, "pass_case", CASE_WEIGHT_P1);
	qa_frame_case_set_weight(pSuite, "fail_case", CASE_WEIGHT_P1);
	qa_frame_case_set_weight(pSuite, "pass_case_again", CASE_WEIGHT_P1);


#if 0
	/*stress_module */
	pModule = qa_frame_add_test_module("stress_module", &error);	
	if(pModule == NULL){		
		printf("Can't add stress_module. error='%s'\n", qa_frame_get_error_str(error));
		retVal = RET_FAIL;
	}else		
		printf("Add stress_module success.error='%s'\n", qa_frame_get_error_str(error));			

	pSuite = qa_frame_add_test_suite(pModule, "stress_test_suite", pass_suite_init, suite_clean, &error);	
	if(pSuite == NULL){		
		printf("Can't add test suite into frame. error='%s'\n", qa_frame_get_error_str(error));
		retVal = RET_FAIL;
	}else		
		printf("Add test suite into module success. error='%s'\n", qa_frame_get_error_str(error));		

	{
		int i;
		char tmp_name[64];
		for(i=0; i<5000; i++){
			sprintf(tmp_name, "stress_pass_%d", i);
			qa_frame_add_test_case(pSuite, tmp_name, pass_case,&error);
			printf("Add case %s into stress suite, return %s.\n", tmp_name, qa_frame_get_error_str(error));
		}

		for(;i<6000; i++){
			sprintf(tmp_name, "stress_crash_%d", i);
			qa_frame_add_test_case(pSuite, tmp_name, crash_case,&error);
			printf("Add case %s into stress suite, return %s.\n", tmp_name, qa_frame_get_error_str(error));
		}

		for(;i<10000; i++){
			sprintf(tmp_name, "stress_fail_%d", i);
			qa_frame_add_test_case(pSuite, tmp_name, fail_case,&error);
			printf("Add case %s into stress suite, return %s.\n", tmp_name, qa_frame_get_error_str(error));
		}		
	}
#endif
	qa_frame_case_interval_time_set(1);

	return retVal;
}
