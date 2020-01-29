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
        printf("<div id=\"chart\"></div>\n<script>\n");
        FILE* script = fopen("chart.js", "r");
        if(script)
        {
            fseek(script, 0, SEEK_END);
            int32_t size = ftell(script);
            rewind(script);
            uint8_t* binary = (uint8_t*)malloc(size);
            size_t read = fread(binary, 1, size, script);
            if(read != size) return;

            binary[size-1] = '\0';
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
        printf("<font style=\"font-family: courier\" color=\"red\">%s</font></br>\n",     gCategories[0]);
        printf("<font style=\"font-family: courier\" color=\"orange\">%s</font></br>\n",  gCategories[1]);
        printf("<font style=\"font-family: courier\" color=\"yellow\">%s</font></br>\n",  gCategories[2]);
        printf("<font style=\"font-family: courier\" color=\"green\">%s</font></br>\n",   gCategories[3]);
        printf("<font style=\"font-family: courier\" color=\"blue\">%s</font></br>\n",    gCategories[4]);
        printf("<font style=\"font-family: courier\" color=\"indigo\">%s</font></br>\n",  gCategories[5]);
        printf("<font style=\"font-family: courier\" color=\"violet\">%s</font></br>\n",  gCategories[6]);
        printf("<font style=\"font-family: courier\" color=\"white\">%s</font></br>\n",   gCategories[7]);
        printf("<font style=\"font-family: courier\" color=\"silver\">%s</font></br>\n",  gCategories[8]);
        printf("<font style=\"font-family: courier\" color=\"gray\">%s</font></br>\n",    gCategories[9]);
        printf("<font style=\"font-family: courier\" color=\"black\">%s</font></br>\n",   gCategories[10]);
        printf("<font style=\"font-family: courier\" color=\"maroon\">%s</font></br>\n",  gCategories[11]);
        printf("<font style=\"font-family: courier\" color=\"olive\">%s</font></br>\n",   gCategories[12]);
        printf("<font style=\"font-family: courier\" color=\"lime\">%s</font></br>\n",    gCategories[13]);
        printf("<font style=\"font-family: courier\" color=\"aqua\">%s</font></br>\n",    gCategories[14]);
        printf("<font style=\"font-family: courier\" color=\"fuchsia\">%s</font></br>\n", gCategories[15]);
        printf("<font style=\"font-family: courier\" color=\"purple\">%s</font></br>\n",  gCategories[16]);
    }

    virtual void statistics()
    {
        int32_t count = sizeof(gCategories)/sizeof(const char*);
        while(count--)
        {
            printf("%s %.2f\n", gCategories[count], gCategoryCount[count]/(double)m_total);
        }
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
