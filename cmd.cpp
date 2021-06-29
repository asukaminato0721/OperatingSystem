#include "FileSystem.h"
#include "Driver.h"
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <iostream>
#include <vector>

//��������
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

#define BLKSIZE 1024  // ���ݿ�Ĵ�С
#define INODESIZE 128 // ��fcb��i�ڵ�Ĵ�С
#define USERLEN 10          //�����û�����������󳤶�

typedef FileControlBlock Inode; // fcb����Inode

// �û�(32B)
typedef struct
{
    char user_name[USERLEN]; // �û���
    char password[USERLEN];  // ����
} User;


char choice;
vector<string> vc_of_str;
string s1, s2;
FCBIndex inum_cur;   //��ǰ�ļ���FCB��                                                                                                                                                   // ��ǰĿ¼
char temp[10 * BLKSIZE];  //д�뻺����                                                                                                                                       // ������
User user;     //���ڻ����ڴ��е��û�                                                                                                                                                 // ��ǰ���û�
//char bitmap[BLKNUM];                                                                                                                                            // λͼ����
//Inode inode_array[INODENUM];                                                                                                                                    // i�ڵ�����
                                                                                                                               // ���ļ�������
//char image_name[10] = "hd.dat";                                                                                                                                 // �ļ�ϵͳ����
FILE* fp;                                                                                                                                                       // ���ļ�ָ��
vector<string> Commands = { "help", "cd", "ls", "mkdir", "touch", "open", "cat", "vi", "close", "rm", "su", "clear", "format", "exit", "rmdir", "info", "copy" }; // 17��

/*����: �û���½����������û��򴴽��û�*/
void login()
{
    char* p;
    int flag;
    char user_name[USERLEN];
    char password[USERLEN];
    char file_name[10] = "user.txt";
    char choice; //ѡ���Ƿ�y/n��
    do
    {
        printf("login:");
        gets_s(user_name);//�û������û���������
        printf("password:");
        p = password;
        while (*p = _getch())//�������룬������
        {
            if (*p == 0x0d) //������س���ʱ��0x0dΪ�س�����ASCII��
            {
                *p = '\0'; //������Ļس���ת���ɿո�
                break;
            }
            printf("*"); //�������������"*"����ʾ
            p++;
        }
        flag = 0;
        if ((fp = fopen(file_name, "r+")) == NULL)//��ȡ�û���Ϣ�ļ�
        {
            printf("\nCan't open file %s.\n", file_name);
            printf("This filesystem is not exist now, it will be create~~~\n");
            format();
            login();
        }
        while (!feof(fp))
        { // feof�ļ�δ��������0
            fread(&user, sizeof(User), 1, fp);
            // �Ѿ����ڵ��û�, ��������ȷ
            if (!strcmp(user.user_name, user_name) && !strcmp(user.password, password))
            {
                fclose(fp);
                printf("\n");
                //CreateDirectory(user.user_name, 0);//�ڸ�Ŀ¼�´����û��ļ��У��������û�ʹ�ã��ļ�ϵͳ����Ϊ����Ӳ�̣���¼ʱ�������´���
                cd(user_name);//�����û��ļ���
                return; //��½�ɹ���ֱ��������½����
            }
            // �Ѿ����ڵ��û�, ���������
            else if (!strcmp(user.user_name, user_name))
            {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1; //����flagΪ1����ʾ����������µ�½
                fclose(fp);
                break;
            }
        }
        if (flag == 0)
        {
            printf("\nThis user is not exist.\n");
            break; //�û������ڣ�������ѭ���������û�
        }
    } while (flag);

    // �������û�����ǰ�û���δע��
    if (flag == 0)
    {
        printf("\nDo you want to creat a new user?(y/n):");
        scanf("%c", &choice);
        // gets_s(temp);
        if ((choice == 'y') || (choice == 'Y'))
        {
            strcpy(user.user_name, user_name);//���û�֮ǰ������û���������ע�����û�
            strcpy(user.password, password);
            fwrite(&user, sizeof(User), 1, fp);//�����û���Ϣд���ļ�
            fclose(fp);//�ر��ļ�
            CreateDirectory(user.user_name, 0);//�ڸ�Ŀ¼�´����û��ļ��У��������û�ʹ��
            cd(user.user_name);//�����û��ļ���
            return;
        }
        if ((choice == 'n') || (choice == 'N'))
            login();
    }
}

//����: �л���ǰ�û�������login��logout
void su(string user_name) {
    char* p;
    int flag;
    //string user_name;
    char password[USERLEN];
    char file_name[10] = "user.txt";
    fp = fopen(file_name, "r");           //��ʼ��ָ�룬���ļ�ϵͳ��ָ��ָ���ļ�ϵͳ���׶�(��ֻ����ʽ���ļ�)
    do {
        //user_name = s2;
        printf("password:");
        p = password;
        while (*p = _getch()) {
            if (*p == 0x0d) { 		//������س���ʱ��0x0dΪ�س�����ASCII��
                *p = '\0'; 			//������Ļس���ת���ɿո�
                break;
            }
            printf("*");   //�������������"*"����ʾ
            p++;
        }
        flag = 0;
        while (!feof(fp)) {
            fread(&user, sizeof(User), 1, fp);
            // �Ѿ����ڵ��û�, ��������ȷ
            if ((user.user_name == user_name) &&
                !strcmp(user.password, password)) {
                fclose(fp);
                printf("\n");
                init();//��inum_cur��ʼ��Ϊ0���ص���Ŀ¼
                cd(user_name);//�����û��ļ���
                return;     //��½�ɹ���ֱ��������½����
            }
            // �Ѿ����ڵ��û�, ���������
            else if ((user.user_name == user_name)) {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1;    //����flagΪ1����ʾ����������µ�½
                fclose(fp);
                break;
            }
        }
        if (flag == 0) {
            printf("\nThis user is not exist.\n");
            break;     //�û������ڣ�ֱ������ѭ����������һ��ָ�������
        }
    } while (flag);
}

//��ʼ��
void init()
{
    // ��ǰĿ¼Ϊ��Ŀ¼
    inum_cur = 0;
}

//�����ļ�·�������ڻ���
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
            FileInfo(temp, &fcb);//�״����ݵ�ǰFCB��
            s = fcb.Name + s;
            s = '/' + s;
            temp = fcb.Parent;
        }
    }
    cout << user.user_name << "@"
        << "8080"
        << ":~" << s << "# ";
}

/*����: ��ʾ��������*/
void help()
{
    printf("command: \n\
    help   ---  ��ʾ�����˵� \n\
    cd     ---  �ı��ļ��� \n\
    clear  ---  ���� \n\
    ls     ---  ��ʾ�ڵ�ǰĿ¼�������ļ����ļ��� \n\
    mkdir  ---  �������ļ���   \n\
    touch  ---  �������ļ� \n\
    cat    ---  �򿪲���ȡ�Ѵ����ļ� \n\
    vi     ---  ���Ѵ����ļ�������д�� \n\
    rm     ---  ɾ���Ѵ����ļ� \n\
    su     ---  �л���ǰ�û� \n\
    format ---  ��ʽ����ǰ�ļ�ϵͳ \n\
    exit   ---  �˳�ϵͳ \n\
    rmdir  ---  ɾ���ļ��� \n\
    info   ---  ��ʾ����ϵͳ��Ϣ \n");
}

//���������������������
// ���: 0-16Ϊϵͳ����, 17Ϊ�������
int analyse()
{
    string s = "";
    s1 = "";
    s2 = "";          // s��ȫ�����룻s1���ڴ����s2���ڴ����
    int tabcount = 0; //���ڼ�¼tab���������
    int res = 0;      //������
    while (1)
    {
        s1 = s; //����Ϊȫ������
        if (s.find(' ') == -1)
            s2 = ""; //���벻���ڿո��޲���
        else
        { //������ڿո񣬱�ʾ���ڲ���
            while (!s1.empty() && s1.back() == ' ')
            {                                       // s1ĩβΪ�ո�(s1.back())����δ��ȡ����
                s1 = s1.substr(0, s1.length() - 1); //��s1���������ȡ��
            }
            while (!s1.empty() > 0 && s1.front() == ' ')
            {                      //���ʼ�ո�ȥ��
                s1 = s1.substr(1); //�ӵ�һ���ַ���ʼ
            }
            if (s1.find(' ') == -1)
                s2 = ""; // s1�޿ո�
            else
                s2 = s1.substr(s1.find_first_of(' ') + 1); //���������ִ���s2
            while (!s2.empty() && s2.back() == ' ')
            { //������ĩβ����ո�ȥ��
                s2 = s2.substr(0, s2.length() - 1);
            }
            while (s2.length() > 0 && s2[0] == ' ')
            { //��������ͷ�ո�ȥ��
                s2 = s2.substr(1);
            }
            s1 = s1.substr(0, s1.find_first_of(' ')); //��whileѭ����ʼ��ȫ������s1������Ϊ����
                                                      //�ڶ��������ǵ�һ���ո��λ�ã�����������
        }
        int ch = _getch(); //�ӿ���̨��ȡһ���ַ���������ʾ����Ļ�ϣ���conio.h��
        if (ch == 8)
        { //�˸�
            if (!s.empty())
            {
                printf("%c", 8);                 //������ǰһ���ַ���λ��
                printf(" ");                     //�ո�ȡ��
                printf("%c", 8);                 //�ٻ�����ǰһ���ַ���λ�ã����ӽ����Ѻ�
                s = s.substr(0, s.length() - 1); //ȥ�����һ���ַ�
            }
        }
        else if (ch == 13)
        { //�س�
            for (res = 0; res < 17; res++)//��δƥ��ɹ�����resΪ17����ʱ���ж��������
            {
                if (s1 == Commands[res])
                    break; //ƥ������
            }
            break; //��������
        }
        else if (ch == 9)
        { // tab
        }
        else if (ch == ' ')
        { //�ո���ԣ�����s��
            printf("%c", ch);
            s.push_back(ch);
        }
        else
        { //�����ַ����ԣ�����s��
            printf("%c", ch);
            s.push_back(ch);
        }
        //���ڴ�������tab
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
    return res; //����������
}

// ����: �л�Ŀ¼(cd .. ���� cd dir1)
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

// bin/xx ��������bin����
// result_cur ����cd�����ļ��ڵ��
// s2 cd �����·�ɴ�
// inum_cur  ��ǰ�ļ��Ľڵ��
int readby(string path)
{ //���ݵ�ǰĿ¼�͵ڶ�������ȷ��ת��ȥ��Ŀ¼
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
    {                                                   // ��·����ÿһ���ļ��д���vector
        v.push_back(s.substr(0, s.find_first_of('/'))); // ��ȡ��1��б��֮ǰ���ַ���
        s = s.substr(s.find_first_of('/') + 1);         //
    }
    if (v.empty())
    { // ˵��û����Ŀ¼��ֱ�ӷ���
        return inum_cur;
    }
    if (v[0].empty())
    { // û���κ��ƶ��������ڵ�ǰ�ļ���
        temp_cur = 0;
    }
    else if (v[0] == "..")
    {                                    // ���ص���һ��Ŀ¼
                                         /*
        if (inode_array[inum_cur].Size > 0) {  // ��ǰ�ڵ�Ľڵ�������0
            temp_cur = inode_array[inum_cur].Parent;
        }
        */
        temp_cur = Find(inum_cur, v[0]); // ������һ��Ŀ¼��Ŀ¼��
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
    { // ���ҵ�cd�������ļ���
        temp_cur = Find(temp_cur, v[count]);
    }
    result_cur = temp_cur;
    return result_cur;
}

void ls(string path)   // pathΪ�����г���ǰ�ļ����µ�ȫ�����ļ�����Ϊ�����г�path·���µ����ļ�
{
    int temp_cur;
    int i = 0;
    if (path.empty())  // pathΪ�գ���ǰ�ļ����µ����ļ�
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

//cmd�����ļ��������ڵ�ǰĿ¼�´����ļ���
void mkdir()
{
    if (s2.empty())
    {
        cout << "Please input directery name\n" << endl;
        return;
    }
    int i, temp_cur;
    string temps1, temps2;
    if (s2.find('/') != -1) {  // Ҫ�������ļ��в��ڵ�ǰĿ¼�£�������·���е�ָ���ļ���
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

// ����: �ڵ�ǰĿ¼�´����ļ�(creat file1)
void touch()
{
    if (s2.length() == 0) {
        printf("Please input filename.\n");
        return;
    }
    int i, temp_cur;
    string temps1, temps2;
    if (s2.find('/') != -1) {  // Ҫ������file���ڵ�ǰĿ¼�£�������·���е�ָ���ļ���
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
    if (s2.find('/') != -1) {  // ����Ĳ����ļ�������·��
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
    int64_t res = ReadFile(file_cur, 0, fcb.Size, buff);//ָ�����ʹ�ָ�����Ͳ������ü�&
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
    if (s2.find('/') != -1) {  // ����Ĳ����ļ�������·��
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

// ����: ɾ���ļ�
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


// ����: ɾ���ļ���
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

// ����: exit�˳��ļ�ϵͳ(quit)
void quit()
{
    char choice;
    printf("Do you want to exist(y/n):");
    scanf("%c", &choice);
    gets_s(temp);
    if ((choice == 'y') || (choice == 'Y')) {
        DismountDisk();//�������е�����д�����
        exit(-1);
    }

}

//cmd�µ�format�����������û��ĵĸ�ʽ��
void format() {
    printf("Are you sure format the fileSystem?(Y/N)?");
    scanf("%c", &choice);
    if ((choice == 'y') || (choice == 'Y')) {
        //���õײ㺯������ʽ�����̣�Ĭ�Ͻ����̳��������Ϣ�����ڴ�
        FormatDisk();
        //����û���Ϣ
        fp = fopen("user.txt", "w+");//��w+��ʽ��������������û���Ϣ�ļ�
        if (fp == NULL)
        {
            printf("Can't create file %s\n", "user.txt");
            exit(-1);
        }
        fclose(fp);
        printf("Filesystem created successful.Please first login!\n");
    }

}

//����ڴ��д��ڵ��û���
void free_user()
{
    int i;
    for (i = 0; i < USERLEN; i++)//ѭ��
        user.user_name[i] = '\0';
}

void info()
{
    PrintDiskInfo();
}

//copy a-b �½�b�ļ�������a�ļ����ݣ����޸�
void copy(string path) {


}

// ����: ��ʾ����
void errcmd()
{
    printf("Command Error!!!\n");
}

// ����: ѭ��ִ���û����������, ֱ��logout
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
            rmdir();//ɾ���ļ���
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

// ������
int main(void)
{
    initDisk();//��ʼ���ļ�ָ��ȣ�ʹ�����в����ܹ�д�����
    //format();
    bool flag = LoadDisk();//���ش��̣������̵ĳ�����ȿ�����Ϣ�����ڴ棬û��format()������Ҫ����һ��
    if (flag == false)
    {
        printf("lodedisk file!\n");
        return 0;
    }
    init();
    login();
    command();
    DismountDisk();//�������е�����д�����
    return 0;
}