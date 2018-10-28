#include <stdio.h>
#include <stdint.h>
#include "analyzer.h"

const char* gCategories[] =
{
    "datamov",
    "arith",
    "logical",
    "bit",
    "branch",
    "control",
    "stack",
    "conver",
    "binary",
    "decimal",
    "shftrot",
    "cond",
    "break",
    "string",
    "inout",
    "flgctrl",
    "segreg"
};

uint64_t gCategoryCount[sizeof(gCategories)/sizeof(const char*)];

class categoryAnalyzer: public analyzer
{
private:

    uint64_t m_total;

public:

    virtual void report()
    {
        FILE* script = fopen("chart.js", "r");
        if(script)
        {
            fseek(script, 0, SEEK_END);
            int32_t size = ftell(script);
            rewind(script);
            uint8_t* binary = (uint8_t*)malloc(size);
            size_t read = fread(binary, 1, size, script);
            if(read != size) return;

            printf("%s\n", binary);
            free(binary);
        }

        printf("addRange(%.0f, \"red\");\n",     (gCategoryCount[0]/(double)m_total) * 100);
        printf("addRange(%.0f, \"orange\");\n",  (gCategoryCount[1]/(double)m_total) * 100);
        printf("addRange(%.0f, \"yellow\");\n",  (gCategoryCount[2]/(double)m_total) * 100);
        printf("addRange(%.0f, \"green\");\n",   (gCategoryCount[3]/(double)m_total) * 100);
        printf("addRange(%.0f, \"blue\");\n",    (gCategoryCount[4]/(double)m_total) * 100);
        printf("addRange(%.0f, \"indigo\");\n",  (gCategoryCount[5]/(double)m_total) * 100);
        printf("addRange(%.0f, \"violet\");\n",  (gCategoryCount[6]/(double)m_total) * 100);
        printf("addRange(%.0f, \"white\");\n",   (gCategoryCount[7]/(double)m_total) * 100);
        printf("addRange(%.0f, \"silver\");\n",  (gCategoryCount[8]/(double)m_total) * 100);
        printf("addRange(%.0f, \"gray\");\n",    (gCategoryCount[9]/(double)m_total) * 100);
        printf("addRange(%.0f, \"black\");\n",   (gCategoryCount[10]/(double)m_total) * 100);
        printf("addRange(%.0f, \"maroon\");\n",  (gCategoryCount[11]/(double)m_total) * 100);
        printf("addRange(%.0f, \"olive\");\n",   (gCategoryCount[12]/(double)m_total) * 100);
        printf("addRange(%.0f, \"lime\");\n",    (gCategoryCount[13]/(double)m_total) * 100);
        printf("addRange(%.0f, \"aqua\");\n",    (gCategoryCount[14]/(double)m_total) * 100);
        printf("addRange(%.0f, \"fuchsia\");\n", (gCategoryCount[15]/(double)m_total) * 100);
        printf("addRange(%.0f, \"purple\");\n",  (gCategoryCount[16]/(double)m_total) * 100);

        printf("</script>\n\n");

        //key
        printf("<font color=\"red\">%s</font></br>\n",     gCategories[0]);
        printf("<font color=\"orange\">%s</font></br>\n",  gCategories[1]);
        printf("<font color=\"yellow\">%s</font></br>\n",  gCategories[2]);
        printf("<font color=\"green\">%s</font></br>\n",   gCategories[3]);
        printf("<font color=\"blue\">%s</font></br>\n",    gCategories[4]);
        printf("<font color=\"indigo\">%s</font></br>\n",  gCategories[5]);
        printf("<font color=\"violet\">%s</font></br>\n",  gCategories[6]);
        printf("<font color=\"white\">%s</font></br>\n",   gCategories[7]);
        printf("<font color=\"silver\">%s</font></br>\n",  gCategories[8]);
        printf("<font color=\"gray\">%s</font></br>\n",    gCategories[9]);
        printf("<font color=\"black\">%s</font></br>\n",   gCategories[10]);
        printf("<font color=\"maroon\">%s</font></br>\n",  gCategories[11]);
        printf("<font color=\"olive\">%s</font></br>\n",   gCategories[12]);
        printf("<font color=\"lime\">%s</font></br>\n",    gCategories[13]);
        printf("<font color=\"aqua\">%s</font></br>\n",    gCategories[14]);
        printf("<font color=\"fuchsia\">%s</font></br>\n", gCategories[15]);
        printf("<font color=\"purple\">%s</font></br>\n",  gCategories[16]);
    }

    virtual void console()
    {
    }

    virtual void analyze(uint64_t address, const isa_instr* instruction)
    {
        int32_t count = sizeof(gCategories)/sizeof(const char*);
        while(count--)
        {
            if(!strcmp(instruction->m_subgroup, gCategories[count]))
            {
                gCategoryCount[count]++;
                break;
            }
        }

        m_total = 0; //reset
        count = sizeof(gCategories)/sizeof(const char*);
        while(count--)
        {
            m_total += gCategoryCount[count];
        }
    }
};
