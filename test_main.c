/*----------------------------------------------------------
| Include definition below this line
+----------------------------------------------------------*/
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <glib.h>
#include <glib-object.h>

#include "frame.h"
#include "example_component/test_cases.h"


/*----------------------------------------------------------
| Macro definition below this line
+----------------------------------------------------------*/


/*----------------------------------------------------------
| Type definition below this line
+----------------------------------------------------------*/
typedef struct
{
	TestModule *pCurModule;
	TestSuite *pCurSuite;
	TestCase *pCurCase;
	guint uiRandSeed;
	guint uiMaxSteps;
}system_state;

/*----------------------------------------------------------
| Global vars definition below this line
+----------------------------------------------------------*/
static GMainLoop *gMainLoop = NULL;

system_state g_sys_state;

/*----------------------------------------------------------
| Functions definition below this line
+----------------------------------------------------------*/
CaseResult test_appman_init(void);


static void case_tree_init(void)
{
	qa_frame_initialize_test_registry();


	/*Below modules init function will create cases tree*/	
	test_module_init();
	test_appman_init();
	tss_test_module_init();
	uam_test_module_init();
	rm_test_module_init();
    test_mpm_sfs_init();

}

static void parse_case_ids(gchar *cases)
{
	QaError error;
	gint id_num=0, level=1;
	char *str_p = cases;


	/*<componet_num>.<suite_num>.<case_num>*/
	while(TRUE){
		if(g_ascii_isdigit(*str_p)){
			id_num=id_num*10+(*str_p-'0');
		}else if((*str_p=='.') || (*str_p=='\0')){
			if(level==1){
				QA_PRINT(3, ("test module ID: %d\n", id_num));
				g_sys_state.pCurModule = qa_frame_find_module_by_ID(id_num, &error);
				QA_PRINT(2, ("Find module return %x, error=%s\n", (guint)g_sys_state.pCurModule, qa_frame_get_error_str(error)));
				assert(g_sys_state.pCurModule!=NULL);
			}else if(level==2){
				QA_PRINT(3, ("test suite ID: %d\n", id_num));
				g_sys_state.pCurSuite = qa_frame_find_suite_by_ID(g_sys_state.pCurModule, id_num, &error);
				QA_PRINT(2, ("Find Suite return %x, error=%s\n", (guint)g_sys_state.pCurSuite, qa_frame_get_error_str(error)));
				assert(g_sys_state.pCurSuite!=NULL);
			}else if(level==3){
				QA_PRINT(3, ("test case ID: %d\n", id_num));
				g_sys_state.pCurCase = qa_frame_find_case_by_ID(g_sys_state.pCurSuite, id_num, &error);
				QA_PRINT(2, ("Find Case return %x, error=%s\n", (guint)g_sys_state.pCurCase, qa_frame_get_error_str(error)));
				assert(g_sys_state.pCurCase!=NULL);
			}else{
				QA_PRINT_FAIL(("Run in wrong way.\n"));
				assert(FALSE);
			}
			id_num = 0;
			level++;

			if(*str_p=='\0')
				break;
		}else{
			QA_PRINT_FAIL(("The case number is wrong, please query case list to identify case number.\n"));
			return;
		}

		str_p++;
	}

}

static void random_run(void)
{
	QaError error;

	printf("\nrandom run cases, max_steps=%d, rand_seed=%d.\n", g_sys_state.uiMaxSteps, g_sys_state.uiRandSeed);

	if(g_sys_state.pCurModule == NULL)
		error = qa_frame_random_run_all_cases(g_sys_state.uiMaxSteps, g_sys_state.uiRandSeed);
	else if(g_sys_state.pCurSuite == NULL)
		error = qa_frame_random_run_test_module(g_sys_state.pCurModule->pName, g_sys_state.uiMaxSteps, g_sys_state.uiRandSeed);
	else if(g_sys_state.pCurCase == NULL)
		error = qa_frame_random_run_test_suite(g_sys_state.pCurModule->pName, g_sys_state.pCurSuite->pName, g_sys_state.uiMaxSteps, g_sys_state.uiRandSeed);
	else
		error = qa_frame_random_run_test_case(g_sys_state.pCurModule->pName, g_sys_state.pCurSuite->pName, g_sys_state.pCurCase->pName, g_sys_state.uiMaxSteps, g_sys_state.uiRandSeed);
	
}

static void print_help_page(void)
{
	printf("==MCA test suite==\nManual\n");
	printf("\trun case\n\t\tMCA_TS all\tOr\n");
	printf("\t\tMCA_TS <case_list>\n");
	printf("\t\tExample:\n");
	printf("\t\t\tMCA_TS 1.2.1\t\t--run single case 1.2.1\n");
	printf("\t\t\tMCA_TS 1.2.1,2.1,3\t--run case 1.2.1, suite 2.1 and module 3\n");
	printf("\tquery case list\n\t\tMCA_TS list\tOr\n");
	printf("\t\tMCA_TS list <case_list>\n");
	printf("\trandom run case(s)\n");
	printf("\t\tMCA_TS random [-t times] [-r rand seed] <case_list>\n");
	printf("\t\t -t\tspecify how many times it would repeated. If no this option, the repeated times is default to 1000.\n");
	printf("\t\t -r\tspecify the rand seed. If no this option, the rand seed is default set to 60.\n");
	printf("\tAdd priority filter.\n");
	printf("\t\tMCA_TS <case_list> [-priority P1 or P2 or P3]\n");
	printf("\t\tExample:\n");
	printf("\t\t\tMCA_TS all -priority P1\t\t--run all the P1 cases.\n");
	printf("\t\t\tMCA_TS 1.2,2 -priority P3\t--run all the P3 cases of suite 1.2 and module 2.\n");
}

static void parse_command_option(gint argc, gchar **argv)
{
	if(argc<2 || g_strcmp0("help", argv[1])==0){
		print_help_page();
	}else{
		if(g_strcmp0("list", argv[1])==0){
			case_tree_init();
			if(argc==2){
				qa_frame_print_case_list();
			}else if(argc==3){
				qa_frame_print_case_list_by_ID(argv[2]);
			}else
				print_help_page();
		}else if(g_strcmp0("random", argv[1])==0){
			int i=2;
			while(i<argc){
				if(g_strcmp0("-t", argv[i])==0){
					if((i+1)<argc && g_ascii_isdigit(argv[i+1][0]))
						g_sys_state.uiMaxSteps = ((guint)g_ascii_strtod(argv[i+1], NULL));
					else{
						printf("Bad parameter for '-t' option.\n");
						g_sys_state.uiMaxSteps = 0;
						break;
					}
					i = i+2;
				}else if(g_strcmp0("-r", argv[i])==0){
					if((i+1)<argc && g_ascii_isdigit(argv[i+1][0]))
						g_sys_state.uiRandSeed = ((guint)g_ascii_strtod(argv[i+1], NULL));
					else{
						printf("Bad parameter for '-r' option.\n");
						g_sys_state.uiRandSeed = 0;
						break;
					}
					i = i+2;				
				}else if(g_ascii_isdigit(argv[i][0])){
					case_tree_init();
					parse_case_ids(argv[i]);
					break;
				}else{
					printf("Bad parameters. see manual page.\n");
					break;
				}
			}

			if(g_sys_state.pCurModule!=NULL 
				&& g_sys_state.uiMaxSteps>0 
				&& g_sys_state.uiRandSeed>0)
				random_run();
			else
				printf("Bad parameters.\n");
		}else if(g_strcmp0("all", argv[1])==0){
			int priority=CASE_WEIGHT_P1|CASE_WEIGHT_P2|CASE_WEIGHT_P3;
			if(argc==4 && g_strcmp0("-priority", argv[2])==0){
				if(g_strcmp0("P1", argv[3])==0 || g_strcmp0("p1", argv[3])==0)
					priority = CASE_WEIGHT_P1;
				else if(g_strcmp0("P2", argv[3])==0 || g_strcmp0("p2", argv[3])==0)
					priority = CASE_WEIGHT_P2;
				else if(g_strcmp0("P3", argv[3])==0 || g_strcmp0("p3", argv[3])==0)
					priority = CASE_WEIGHT_P3;
				else{
					printf("The parameters of priority is bad, see manual page.\n");
					return;
				}
			}
			printf("START ===[MCA_TS]===\n[test_bed][MCA_TS][Init][starting MCA_TS][end]\n");
			case_tree_init();
			qa_frame_run_all_cases(priority);
			printf("END ====[MCA_TS]=====\n[test_bed][MCA_TS][Exit][O_tb_exit()][end]\n");
			
		}else{
			int priority=CASE_WEIGHT_P1|CASE_WEIGHT_P2|CASE_WEIGHT_P3;
			if(argc==4 && g_strcmp0("-priority", argv[2])==0){
				if(g_strcmp0("P1", argv[3])==0 || g_strcmp0("p1", argv[3])==0)
					priority = CASE_WEIGHT_P1;
				else if(g_strcmp0("P2", argv[3])==0 || g_strcmp0("p2", argv[3])==0)
					priority = CASE_WEIGHT_P2;
				else if(g_strcmp0("P3", argv[3])==0 || g_strcmp0("p3", argv[3])==0)
					priority = CASE_WEIGHT_P3;
				else{
					printf("The parameters of priority is bad, see manual page.\n");
					return;
				}
			}
			QA_PRINT(3, ("priority=%d", priority));
			printf("START ===[MCA_TS]===\n[test_bed][MCA_TS][Init][starting MCA_TS][end]\n");
			case_tree_init();
			qa_frame_run_case_by_list(argv[1], priority);
			printf("END ====[MCA_TS]=====\n[test_bed][MCA_TS][Exit][O_tb_exit()][end]\n");
		}
	}

}



gint main(gint argc, gchar **argv)
{
	g_type_init();		
	if (!g_thread_supported ()) {        
		g_thread_init (NULL);    
	}
	
    /*Create a glib main loop*/	
	gMainLoop = g_main_loop_new (NULL, FALSE);	
	if (NULL==gMainLoop) {		
		QA_PRINT_FAIL(("Failed to create the glib main loop\n"));		
		exit (1);	
	}

	g_sys_state.pCurCase = NULL;
	g_sys_state.pCurModule = NULL;
	g_sys_state.pCurSuite = NULL;
	g_sys_state.uiMaxSteps = 1000;
	g_sys_state.uiRandSeed = 60;

	parse_command_option(argc, argv);


	/*Doing frame clean up*/
	qa_frame_cleanup_registry();

	if (gMainLoop){
		g_main_loop_quit (gMainLoop);
		exit(0);
	}

	/*Start-up the main loop*/	
	g_main_loop_run (gMainLoop);
	return 0;
}

