// Vir.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int main()
{
	char buf[] = "i am moonagirl.";
	char buf1[100];
	FILE *file = NULL;
	errno_t err = fopen_s(&file,"C:\\###000trap\\tmplazy.txt", "w+");
	if (err)
	{
		puts("open file failed!!\n");
		return 0;
	}
	fwrite(buf, 1, 15, file);
	fclose(file);
	err = fopen_s(&file, "C:\\###000trap\\tmplazy.txt", "r+");
	if (err)
	{
		puts("open file failed!!\n");
		return 0;
	}
	fread(buf1, 1, 15, file);
	fclose(file);
	puts(buf1);
	system("pause");
    return 0;
}