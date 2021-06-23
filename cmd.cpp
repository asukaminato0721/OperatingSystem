#include "FileSystem.h"

#define BLKSIZE 1024  // 数据块的大小
#define BLKNUM 512    // 数据块的块数
#define INODESIZE 128 // （fcb）i节点的大小
#define INODENUM 32   // i节点的数目
#define FILENUM 8     // 打开文件表的数目

/*
super.BlockNum
super.FCBNum
super.DataBlockNum
*/

typedef FileControlBlock Inode; // fcb别名Inode

// 用户(20B)
typedef struct {
    char user_name[10]; // 用户名
    char password[10];  // 密码
} User;

// 打开文件表(16B)
typedef struct {
    short inum;         // i节点号
    char file_name[10]; // 文件名
    short mode;         // 读写模式(1:read, 2:write,
                        //         3:read and write)
    short offset;       // 偏移量
} File_table;

char choice;
vector<string> vc_of_str;
string s1, s2;
int inum_cur;                                                                                                                                                   // 当前目录
char temp[2 * BLKSIZE];                                                                                                                                         // 缓冲区
User user;                                                                                                                                                      // 当前的用户
char bitmap[BLKNUM];                                                                                                                                            // 位图数组
Inode inode_array[INODENUM];                                                                                                                                    // i节点数组
File_table file_array[FILENUM];                                                                                                                                 // 打开文件表数组
char image_name[10] = "hd.dat";                                                                                                                                 // 文件系统名称
FILE *fp;                                                                                                                                                       // 打开文件指针
const string Commands[] = {"help", "cd", "ls", "mkdir", "touch", "open", "cat", "vi", "close", "rm", "su", "clear", "format", "exit", "rmdir", "info", "copy"}; // 17个

/*功能: 用户登陆，如果是新用户则创建用户*/
void login() {
    char *p;
    int flag;
    char user_name[10];
    char password[10];
    char file_name[10] = "user.txt";
    char choice; //选择是否（y/n）
    do {
        printf("login:");
        gets_s(user_name);
        printf("password:");
        p = password;
        while (*p = _getch()) {
            if (*p == 0x0d) //当输入回车键时，0x0d为回车键的ASCII码
            {
                *p = '\0'; //将输入的回车键转换成空格
                break;
            }
            printf("*"); //将输入的密码以"*"号显示
            p++;
        }
        flag = 0;
        if ((fp = fopen(file_name, "r+")) == NULL) {
            printf("\nCan't open file %s.\n", file_name);
            printf("This filesystem is not exist now, it will be create~~~\n");
            FormatDisk();
            login();
        }
        while (!feof(fp)) { // feof文件未结束返回0
            fread(&user, sizeof(User), 1, fp);
            // 已经存在的用户, 且密码正确
            if (!strcmp(user.user_name, user_name) && !strcmp(user.password, password)) {
                fclose(fp);
                printf("\n");
                return; //登陆成功，直接跳出登陆函数
            }
            // 已经存在的用户, 但密码错误
            else if (!strcmp(user.user_name, user_name)) {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1; //设置flag为1，表示密码错误，重新登陆
                fclose(fp);
                break;
            }
        }
        if (flag == 0) {
            printf("\nThis user is not exist.\n");
            break; //用户不存在，先跳出循环创建新用户
        }
    } while (flag);

    // 创建新用户
    if (flag == 0) {
        printf("\nDo you want to creat a new user?(y/n):");
        scanf("%c", &choice);
        // gets_s(temp);
        if ((choice == 'y') || (choice == 'Y')) {
            strcpy(user.user_name, user_name);
            strcpy(user.password, password);
            fwrite(&user, sizeof(User), 1, fp);
            fclose(fp);
            return;
        }
        if ((choice == 'n') || (choice == 'N'))
            login();
    }
}

//初始化
void init() {
    int i;
    // 当前目录为根目录
    inum_cur = 0;
    // 初始化打开文件表
    for (i = 0; i < FILENUM; i++)
        file_array[i].inum = -1;
}

//设置文件路径，用于回显
void pathset() {
    string s;
    if (inum_cur == 0)s = "";
    else {
        FCBIndex temp = inum_cur;
        FileControlBlock* fcb;
        while (temp != 0)
        {
            Getfcb(temp, fcb);
            s = fcb->Name + s;
            s = '/' + s;
            temp = fcb->Parent;
        }
    }
    cout << user.user_name << "@" << "4423" << ":~" << s << "# ";
}

/*功能: 显示帮助命令*/
void help() {
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
int analyse() {
    string s = "";
    s1 = "";
    s2 = "";          // s存全部输入；s1用于存命令；s2用于存参数
    int tabcount = 0; //用于记录tab键输入个数
    int res = 0;      //命令编号
    while (1) {
        s1 = s; //更新为全部输入
        if (s.find(' ') == -1)
            s2 = "";                                                //输入不存在空格，无参数
        else {                                                      //输入存在空格，表示存在参数
            while (s1.length() > 0 && s1[s1.length() - 1] == ' ') { // s1末尾为空格，还未读取参数
                s1 = s1.substr(0, s1.length() - 1);                 //将s1的命令部分提取出
            }
            while (s1.length() > 0 && s1[0] == ' ') { //命令开始空格去掉
                s1 = s1.substr(1);                    //从第一个字符开始
            }
            if (s1.find(' ') == -1)
                s2 = ""; // s1无空格
            else
                s2 = s1.substr(s1.find_first_of(' ') + 1);          //将参数部分传入s2
            while (s2.length() > 0 && s2[s2.length() - 1] == ' ') { //将参数末尾多余空格去掉
                s2 = s2.substr(0, s2.length() - 1);
            }
            while (s2.length() > 0 && s2[0] == ' ') { //将参数开头空格去除
                s2 = s2.substr(1);
            }
            s1 = s1.substr(0, s1.find_first_of(' ')); //将while循环开始的全部输入s1，更新为命令
                                                      //第二个参数是第一个空格的位置，与命令长度相等
        }
        int ch = _getch(); //从控制台读取一个字符，但不显示在屏幕上，在conio.h中
        if (ch == 8) {     //退格
            if (!s.empty()) {
                printf("%c", 8);                 //回退至前一个字符的位置
                printf(" ");                     //空格取代
                printf("%c", 8);                 //再回退至前一个字符的位置，增加交互友好
                s = s.substr(0, s.length() - 1); //去掉最后一个字符
            }
        } else if (ch == 13) { //回车
            for (res = 0; res < 17; res++) {
                if (s1 == Commands[res])
                    break; //匹配命令
            }
            break;            //结束输入
        } else if (ch == 9) { // tab

        } else if (ch == ' ') { //空格回显，存入s中
            printf("%c", ch);
            s.push_back(ch);
        } else { //其他字符回显，存入s中
            printf("%c", ch);
            s.push_back(ch);
        }
        //用于处理按两次tab
        if (ch == 9) {
            tabcount++;
        } else {
            tabcount = 0;
        }
    }
    if (s1 == "") {
        return -1;
    }
    printf("\n");
    return res; //返回命令编号
}


// 功能: 切换目录(cd .. 或者 cd dir1)
void cd(string path)
{
	int temp_cur;
	if (path.length() == 0) {
		temp_cur = 0;
	}
	else {
		if (path[path.length() - 1] != '/')  path += '/';
		temp_cur = readby();
	}
	if (temp_cur != -1) {
		inum_cur = temp_cur;
	}
	else {
		cout << "No Such Directory" << endl;
	}
}

// bin/xx 给出进入bin即可
// result_cur 最终cd到的文件节点号
// s2 cd 后面的路由串
// inum_cur  当前文件的节点号
int readby(string path) {	//根据当前目录和第二个参数确定转过去的目录
	int result_cur = 0; string s = path;
	if (s.find('/') != -1) {
		s = s.substr(0, s.find_last_of('/') + 1);
	}
	else {
		s = "";
	}
	int temp_cur = inum_cur;
	vector<string> v;
	while (s.find('/') != -1) {  // 将路径的每一级文件夹存入vector
		v.push_back(s.substr(0, s.find_first_of('/')));  // 截取第1个斜杠之前的字符串
		s = s.substr(s.find_first_of('/') + 1);  // 
	}
	if (v.size() == 0) { // 说明没有子目录，直接返回
		return inum_cur;
	}
	if (v[0].length() == 0) { // 没有任何移动，依旧在当前文件夹
		temp_cur = 0;
	}
	else if (v[0] == "..") { // 返回到上一级目录
		if (inode_array[inum_cur].Size > 0) {  // 当前节点的节点数大于0
			temp_cur = inode_array[inum_cur].Parent;
		}
	}
	else {
		int i;
		/*
		for (i = 0; i < INODENUM; i++) {
			if ((inode_array[i].inum > 0) &&
				(inode_array[i].iparent == inum_cur) &&  // 第i个节点的父节点是当前节点
				(inode_array[i].type == 'd') &&
				inode_array[i].file_name == v[0]) {  // 第i个节点的文件名是cd到的第一级文件名
				break;  // 找到了第一级cd文件
			}
		}
		*/
		i = Find(inum_cur, v[0]);
		if (i == INODENUM) { // 全部遍历完了，依旧没有，即没有该文件路径
			return -1;
		}
		else {
			temp_cur = i;  // 找到
		}
	}
	int i;
	for (unsigned int count = 1; count < v.size(); count++) { // 逐级找到cd的最终文件夹
		i = Find(inum_cur, v[count]);
		if (i == INODENUM) {
			return -1;
		}
		else {
			temp_cur = i;
		}
	}
	result_cur = temp_cur;
	return result_cur;
}

void mkdir() {
    int i;
    if (s2.length() == 0) {
        cout << "Please input name" << endl;
        return;
    }
    else {

    }
}

// 功能: 显示错误
void errcmd()
{
    printf("Command Error!!!\n");
}

// 功能: 循环执行用户输入的命令, 直到logout
//  0"help", 1"cd", 2"ls", 3"mkdir", 4"touch", 5"open",6"cat", 7"vi", 8"close", 9"rm", 10"su", 11"clear", 12"format",13"exit",14"rmdir",15"info",16"copy"
void command(void) {
    system("cls");
    do {
        pathset();
        switch (analyse()) {
        case -1:
            printf("\n");
            break;
        case 0:
            help();
            break;
        case 1:
            cd();
            break;
        case 2:
            ls();
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
            vi();
            break;
        case 8:
            // close();
            break;
        case 9:
            rm();
            break;
        case 10:
            su();
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
            rmdir();
            break;
        case 15:
            info();
            break;
        case 16:
            copy();
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
int main(void) {
    // format();
    login();
    init();
    command();
    return 0;
}