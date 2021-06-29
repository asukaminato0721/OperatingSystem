#include "FileSystem.h"
#include "Driver.h"
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <iostream>
#include <vector>

//声明函数
void login();
void su(string user_name);
void init();
void pathset();
void help();
int analyse();
void cd(string path);
int readby(string path);
void ls(string path);
void mkdir();
void touch();
void cat();
void vi();
void rm();
void rmdir();
void quit();
void format();
void free_user();
void info();
void copy(string path);
void errcmd();
void command();

using namespace std;

#define BLKSIZE 1024  // 数据块的大小
#define INODESIZE 128 // （fcb）i节点的大小
#define USERLEN 10          //设置用户名和密码最大长度

typedef FileControlBlock Inode; // fcb别名Inode

// 用户(32B)
typedef struct
{
    char user_name[USERLEN]; // 用户名
    char password[USERLEN];  // 密码
} User;


char choice;
vector<string> vc_of_str;
string s1, s2;
FCBIndex inum_cur;   //当前文件夹FCB号                                                                                                                                                   // 当前目录
char temp[10 * BLKSIZE];  //写入缓冲区                                                                                                                                       // 缓冲区
User user;     //用于缓存内存中的用户                                                                                                                                                 // 当前的用户
//char bitmap[BLKNUM];                                                                                                                                            // 位图数组
//Inode inode_array[INODENUM];                                                                                                                                    // i节点数组
                                                                                                                               // 打开文件表数组
//char image_name[10] = "hd.dat";                                                                                                                                 // 文件系统名称
FILE* fp;                                                                                                                                                       // 打开文件指针
vector<string> Commands = { "help", "cd", "ls", "mkdir", "touch", "open", "cat", "vi", "close", "rm", "su", "clear", "format", "exit", "rmdir", "info", "copy" }; // 17个

/*功能: 用户登陆，如果是新用户则创建用户*/
void login()
{
    char* p;
    int flag;
    char user_name[USERLEN];
    char password[USERLEN];
    char file_name[10] = "user.txt";
    char choice; //选择是否（y/n）
    do
    {
        printf("login:");
        gets_s(user_name);//用户输入用户名，回显
        printf("password:");
        p = password;
        while (*p = _getch())//输入密码，不回显
        {
            if (*p == 0x0d) //当输入回车键时，0x0d为回车键的ASCII码
            {
                *p = '\0'; //将输入的回车键转换成空格
                break;
            }
            printf("*"); //将输入的密码以"*"号显示
            p++;
        }
        flag = 0;
        if ((fp = fopen(file_name, "r+")) == NULL)//读取用户信息文件
        {
            printf("\nCan't open file %s.\n", file_name);
            printf("This filesystem is not exist now, it will be create~~~\n");
            format();
            login();
        }
        while (!feof(fp))
        { // feof文件未结束返回0
            fread(&user, sizeof(User), 1, fp);
            // 已经存在的用户, 且密码正确
            if (!strcmp(user.user_name, user_name) && !strcmp(user.password, password))
            {
                fclose(fp);
                printf("\n");
                //CreateDirectory(user.user_name, 0);//在根目录下创建用户文件夹，仅供该用户使用，文件系统更改为存入硬盘，登录时无需重新创建
                cd(user_name);//进入用户文件夹
                return; //登陆成功，直接跳出登陆函数
            }
            // 已经存在的用户, 但密码错误
            else if (!strcmp(user.user_name, user_name))
            {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1; //设置flag为1，表示密码错误，重新登陆
                fclose(fp);
                break;
            }
        }
        if (flag == 0)
        {
            printf("\nThis user is not exist.\n");
            break; //用户不存在，先跳出循环创建新用户
        }
    } while (flag);

    // 创建新用户，当前用户名未注册
    if (flag == 0)
    {
        printf("\nDo you want to creat a new user?(y/n):");
        scanf("%c", &choice);
        // gets_s(temp);
        if ((choice == 'y') || (choice == 'Y'))
        {
            strcpy(user.user_name, user_name);//将用户之前输入的用户名和密码注册新用户
            strcpy(user.password, password);
            fwrite(&user, sizeof(User), 1, fp);//将新用户信息写入文件
            fclose(fp);//关闭文件
            CreateDirectory(user.user_name, 0);//在根目录下创建用户文件夹，仅供该用户使用
            cd(user.user_name);//进入用户文件夹
            return;
        }
        if ((choice == 'n') || (choice == 'N'))
            login();
    }
}

//功能: 切换当前用户包含了login和logout
void su(string user_name) {
    char* p;
    int flag;
    //string user_name;
    char password[USERLEN];
    char file_name[10] = "user.txt";
    fp = fopen(file_name, "r");           //初始化指针，将文件系统的指针指向文件系统的首端(以只读方式打开文件)
    do {
        //user_name = s2;
        printf("password:");
        p = password;
        while (*p = _getch()) {
            if (*p == 0x0d) { 		//当输入回车键时，0x0d为回车键的ASCII码
                *p = '\0'; 			//将输入的回车键转换成空格
                break;
            }
            printf("*");   //将输入的密码以"*"号显示
            p++;
        }
        flag = 0;
        while (!feof(fp)) {
            fread(&user, sizeof(User), 1, fp);
            // 已经存在的用户, 且密码正确
            if ((user.user_name == user_name) &&
                !strcmp(user.password, password)) {
                fclose(fp);
                printf("\n");
                init();//将inum_cur初始化为0，回到根目录
                cd(user_name);//进入用户文件夹
                return;     //登陆成功，直接跳出登陆函数
            }
            // 已经存在的用户, 但密码错误
            else if ((user.user_name == user_name)) {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1;    //设置flag为1，表示密码错误，重新登陆
                fclose(fp);
                break;
            }
        }
        if (flag == 0) {
            printf("\nThis user is not exist.\n");
            break;     //用户不存在，直接跳出循环，进行下一条指令的输入
        }
    } while (flag);
}

//初始化
void init()
{
    // 当前目录为根目录
    inum_cur = 0;
}

//设置文件路径，用于回显
void pathset()
{
    string s;
    if (inum_cur == 0)
        s = "";
    else
    {
        FCBIndex temp = inum_cur;
        FileControlBlock fcb;
        while (temp != 0)
        {
            FileInfo(temp, &fcb);//首次依据当前FCB号
            s = fcb.Name + s;
            s = '/' + s;
            temp = fcb.Parent;
        }
    }
    cout << user.user_name << "@"
        << "8080"
        << ":~" << s << "# ";
}

/*功能: 显示帮助命令*/
void help()
{
    printf("command: \n\
    help   ---  显示帮助菜单 \n\
    cd     ---  改变文件夹 \n\
    clear  ---  清屏 \n\
    ls     ---  显示在当前目录下所有文件及文件夹 \n\
    mkdir  ---  创建新文件夹   \n\
    touch  ---  创建新文件 \n\
    cat    ---  打开并读取已存在文件 \n\
    vi     ---  打开已存在文件并往里写入 \n\
    rm     ---  删除已存在文件 \n\
    su     ---  切换当前用户 \n\
    format ---  格式化当前文件系统 \n\
    exit   ---  退出系统 \n\
    rmdir  ---  删除文件夹 \n\
    info   ---  显示整个系统信息 \n");
}

//分析函数，分析命令及参数
// 结果: 0-16为系统命令, 17为命令错误
int analyse()
{
    string s = "";
    s1 = "";
    s2 = "";          // s存全部输入；s1用于存命令；s2用于存参数
    int tabcount = 0; //用于记录tab键输入个数
    int res = 0;      //命令编号
    while (1)
    {
        s1 = s; //更新为全部输入
        if (s.find(' ') == -1)
            s2 = ""; //输入不存在空格，无参数
        else
        { //输入存在空格，表示存在参数
            while (!s1.empty() && s1.back() == ' ')
            {                                       // s1末尾为空格(s1.back())，还未读取参数
                s1 = s1.substr(0, s1.length() - 1); //将s1的命令部分提取出
            }
            while (!s1.empty() > 0 && s1.front() == ' ')
            {                      //命令开始空格去掉
                s1 = s1.substr(1); //从第一个字符开始
            }
            if (s1.find(' ') == -1)
                s2 = ""; // s1无空格
            else
                s2 = s1.substr(s1.find_first_of(' ') + 1); //将参数部分传入s2
            while (!s2.empty() && s2.back() == ' ')
            { //将参数末尾多余空格去掉
                s2 = s2.substr(0, s2.length() - 1);
            }
            while (s2.length() > 0 && s2[0] == ' ')
            { //将参数开头空格去除
                s2 = s2.substr(1);
            }
            s1 = s1.substr(0, s1.find_first_of(' ')); //将while循环开始的全部输入s1，更新为命令
                                                      //第二个参数是第一个空格的位置，与命令长度相等
        }
        int ch = _getch(); //从控制台读取一个字符，但不显示在屏幕上，在conio.h中
        if (ch == 8)
        { //退格
            if (!s.empty())
            {
                printf("%c", 8);                 //回退至前一个字符的位置
                printf(" ");                     //空格取代
                printf("%c", 8);                 //再回退至前一个字符的位置，增加交互友好
                s = s.substr(0, s.length() - 1); //去掉最后一个字符
            }
        }
        else if (ch == 13)
        { //回车
            for (res = 0; res < 17; res++)//若未匹配成功，则res为17，此时将判断命令错误
            {
                if (s1 == Commands[res])
                    break; //匹配命令
            }
            break; //结束输入
        }
        else if (ch == 9)
        { // tab
        }
        else if (ch == ' ')
        { //空格回显，存入s中
            printf("%c", ch);
            s.push_back(ch);
        }
        else
        { //其他字符回显，存入s中
            printf("%c", ch);
            s.push_back(ch);
        }
        //用于处理按两次tab
        if (ch == 9)
        {
            tabcount++;
        }
        else
        {
            tabcount = 0;
        }
    }
    if (s1 == "")
    {
        return -1;
    }
    printf("\n");
    return res; //返回命令编号
}

// 功能: 切换目录(cd .. 或者 cd dir1)
void cd(string path)
{
    int temp_cur;
    if (path.empty())
    {
        printf("command error!\n");
        return;
    }
    else
    {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
    }
    if (temp_cur != -1)
    {
        inum_cur = temp_cur;
    }
    else
    {
        cout << "No Such Directory\n" << endl;
    }
}

// bin/xx 给出进入bin即可
// result_cur 最终cd到的文件节点号
// s2 cd 后面的路由串
// inum_cur  当前文件的节点号
int readby(string path)
{ //根据当前目录和第二个参数确定转过去的目录
    int result_cur = 0;
    string s = path;
    if (s.find('/') != -1)
    {
        s = s.substr(0, s.find_last_of('/') + 1);
    }
    else
    {
        s = "";
    }
    int temp_cur = inum_cur;
    vector<string> v;
    while (s.find('/') != -1)
    {                                                   // 将路径的每一级文件夹存入vector
        v.push_back(s.substr(0, s.find_first_of('/'))); // 截取第1个斜杠之前的字符串
        s = s.substr(s.find_first_of('/') + 1);         //
    }
    if (v.empty())
    { // 说明没有子目录，直接返回
        return inum_cur;
    }
    if (v[0].empty())
    { // 没有任何移动，依旧在当前文件夹
        temp_cur = 0;
    }
    else if (v[0] == "..")
    {                                    // 返回到上一级目录
                                         /*
        if (inode_array[inum_cur].Size > 0) {  // 当前节点的节点数大于0
            temp_cur = inode_array[inum_cur].Parent;
        }
        */
        temp_cur = Find(inum_cur, v[0]); // 返回上一级目录的目录号
        if (temp_cur == 0) {
            printf("cd failed!\n");
            temp_cur = inum_cur;
        }
    }
    else
    {
        temp_cur = Find(inum_cur, v[0]);
    }
    for (unsigned int count = 1; count < v.size(); count++)
    { // 逐级找到cd的最终文件夹
        temp_cur = Find(temp_cur, v[count]);
    }
    result_cur = temp_cur;
    return result_cur;
}

void ls(string path)   // path为空则列出当前文件夹下的全部子文件，不为空则列出path路径下的子文件
{
    int temp_cur;
    int i = 0;
    if (path.empty())  // path为空，当前文件夹下的子文件
    { //
        temp_cur = inum_cur;
    }
    else
    {
        if (path.back() != '/')
            path += '/';
        temp_cur = readby(path);
        if (temp_cur == -1)
        {
            cout << "No Such Directory\n" << endl;
            return;
        }
    }
    if (temp_cur != -1)
    {
        printf("%12s | %12s | %6s | % 10s\n", "Name", "Size", "FCB", "Physical Address");
        /*vector<FCBIndex> v = GetChildren(temp_cur);
        for (unsigned int count = 0; count < v.size(); count++)
        {
            PrintDir(v[count]);
        }*/
        PrintDir(temp_cur);
    }
}

//cmd创建文件函数，在当前目录下创建文件夹
void mkdir()
{
    if (s2.empty())
    {
        cout << "Please input directery name\n" << endl;
        return;
    }
    int i, temp_cur;
    string temps1, temps2;
    if (s2.find('/') != -1) {  // 要创建的文件夹不在当前目录下，而是在路径中的指定文件下
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        s2 = temps1;
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("No Such Directory\n");
        }
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex index = CreateFile(temps2, temp_cur);
    if (index != -1) {
        printf("Create Directory Successfully!\n");
    }
    else {
        printf("Failed!\n");
    }
}

// 功能: 在当前目录下创建文件(creat file1)
void touch()
{
    if (s2.length() == 0) {
        printf("Please input filename.\n");
        return;
    }
    int i, temp_cur;
    string temps1, temps2;
    if (s2.find('/') != -1) {  // 要创建的file不在当前目录下，而是在路径中的指定文件下
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        s2 = temps1;
        temp_cur = readby(temps1);
        if (temp_cur == -1) {
            printf("No Such Directory\n");
        }
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex index = CreateFile(s2, temp_cur);
    if (index != -1) {
        printf("Create File Successfully!\n");
    }
    else {
        printf("Failed!\n");
    }
}

// an exist file
void cat() {
    int i, inum;
    string temps1, temps2; int temp_cur;
    if (s2.find('/') != -1) {  // 传入的不是文件名而是路径
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        string temps = s2;
        s2 = temps1;
        temp_cur = readby(temps1);
        s2 = temps;
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex file_cur = Find(temp_cur, s2);
    FileControlBlock fcb;
    FileInfo(file_cur, &fcb);
    uint8_t* buff = (uint8_t*)malloc(fcb.Size+1);
    int64_t res = ReadFile(file_cur, 0, fcb.Size, buff);//指针类型传指针类型参数不用加&
    if (res != -1) {
        printf("%s\n", buff);
    }
    else {
        printf("Read Fail!\n");
    }
    free(buff);
}

// open and write something to a particular file
void vi() {
    int i, inum;
    int64_t res;
    string temps1, temps2; int temp_cur;
    char temp[100 * BLKSIZE];
    uint8_t* buff;
    if (s2.find('/') != -1) {  // 传入的不是文件名而是路径
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        string temps = s2;
        s2 = temps1;
        temp_cur = readby(temps1);
        s2 = temps;
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex file_cur = Find(temp_cur, s2);
    FileControlBlock fcb;
    FileInfo(file_cur, &fcb);
    if (fcb.Size == 0) {
        printf("Please input: \n");
        gets_s(temp);
        res = WriteFile(file_cur, 0, strlen(temp), (uint8_t*)(temp));
    }
    else {
        char choice;
        printf("This file already exist data! \n");
        printf("Overwrite or append? input o/a:");
        scanf("%c", &choice);
        if (choice == 'o') {
            printf("Please input: \n");
            gets_s(temp);
            res = WriteFile(file_cur, 0, strlen(temp), (uint8_t*)(temp));
        }
        else if (choice == 'a') {
            printf("Please input: \n");
            gets_s(temp);
            res = WriteFile(file_cur, fcb.Size + 1, strlen(temp), (uint8_t*)(temp));
        }
        else {
            printf("Exit write!\n");
        }
        if (res != -1) {
            printf("Write file successfully!\n");
        }
        else {
            printf("Failed to write!\n");
        }
    }
}

// 功能: 删除文件
void rm(void)
{
    if (s2.length() == 0) {
        printf("This file doesn't exist.\n");
        return;
    }
    int i, temp_cur; string temps1, temps2;
    if (s2.find('/') != -1) {
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        s2 = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex file_cur = Find(temp_cur, temps2);
    bool suc = DeleteFile(file_cur);
    if (suc != -1) {
        printf("Delete Successfully!\n");
    }
    else {
        printf("Delete Failed!\n");
    }
}


// 功能: 删除文件夹
void rmdir(void)
{
    if (s2.length() == 0) {
        printf("This file doesn't exist.\n");
        return;
    }
    int i, temp_cur; string temps1, temps2;
    if (s2.find('/') != -1) {
        temps1 = s2.substr(0, s2.find_last_of('/') + 1);
        temps2 = s2.substr(s2.find_last_of('/') + 1);
        s2 = temps1;
        temp_cur = readby(temps1);
    }
    else {
        temps2 = s2;
        temp_cur = inum_cur;
    }
    FCBIndex file_cur = Find(temp_cur, temps2);
    bool suc = DeleteFile(file_cur);
    if (suc != -1) {
        printf("Delete Successfully!\n");
    }
    else {
        printf("Delete Failed!\n");
    }
}

// 功能: exit退出文件系统(quit)
void quit()
{
    char choice;
    printf("Do you want to exist(y/n):");
    scanf("%c", &choice);
    gets_s(temp);
    if ((choice == 'y') || (choice == 'Y')) {
        DismountDisk();//将缓存中的内容写入磁盘
        exit(-1);
    }

}

//cmd下的format函数，包括用户的的格式化
void format() {
    printf("Are you sure format the fileSystem?(Y/N)?");
    scanf("%c", &choice);
    if ((choice == 'y') || (choice == 'Y')) {
        //调用底层函数，格式化磁盘，默认将磁盘超级块等信息存入内存
        FormatDisk();
        //清除用户信息
        fp = fopen("user.txt", "w+");//以w+方式，若存在则清空用户信息文件
        if (fp == NULL)
        {
            printf("Can't create file %s\n", "user.txt");
            exit(-1);
        }
        fclose(fp);
        printf("Filesystem created successful.Please first login!\n");
    }

}

//清空内存中存在的用户名
void free_user()
{
    int i;
    for (i = 0; i < USERLEN; i++)//循环
        user.user_name[i] = '\0';
}

void info()
{
    PrintDiskInfo();
}

//copy a-b 新建b文件并复制a文件内容，待修复
void copy(string path) {


}

// 功能: 显示错误
void errcmd()
{
    printf("Command Error!!!\n");
}

// 功能: 循环执行用户输入的命令, 直到logout
//  0"help", 1"cd", 2"ls", 3"mkdir", 4"touch", 5"open",6"cat", 7"vi", 8"close", 9"rm", 10"su", 11"clear", 12"format",13"exit",14"rmdir",15"info",16"copy"
void command(void)
{
    system("cls");
    do
    {
        pathset();
        switch (analyse())
        {
        case -1:
            printf("\n");
            break;
        case 0:
            help();
            break;
        case 1:
            cd(s2);
            break;
        case 2:
            ls(s2);
            break;
        case 3:
            mkdir();
            break;
        case 4:
            touch();
            break;
        case 5:
            // open();
            break;
        case 6:
            cat();
            break;
        case 7:
            vi(); // open and write something to a particular file
            break;
        case 8:
            // close();
            break;
        case 9:
            rm();  // delete file
            break;
        case 10:
            su(s2);
            break;
        case 11:
            system("cls");
            break;
        case 12:
            format();
            init();
            free_user();
            login();
            break;
        case 13:
            quit();
            break;
        case 14:
            rmdir();//删除文件夹
            break;
        case 15:
            info();
            break;
        case 16:
            copy(s2);
            break;
        case 17:
            errcmd();
            break;
        default:
            break;
        }
    } while (1);
}

// 主函数
int main(void)
{
    initDisk();//初始化文件指针等，使得所有操作能够写入磁盘
    //format();
    bool flag = LoadDisk();//挂载磁盘，将磁盘的超级块等控制信息放入内存，没有format()，必须要有这一步
    if (flag == false)
    {
        printf("lodedisk file!\n");
        return 0;
    }
    init();
    login();
    command();
    DismountDisk();//将缓存中的内容写入磁盘
    return 0;
}