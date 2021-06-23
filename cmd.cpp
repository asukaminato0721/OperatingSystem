#include "FileSystem.h"

#define BLKSIZE 1024  // ���ݿ�Ĵ�С
#define BLKNUM 512    // ���ݿ�Ŀ���
#define INODESIZE 128 // ��fcb��i�ڵ�Ĵ�С
#define INODENUM 32   // i�ڵ����Ŀ
#define FILENUM 8     // ���ļ������Ŀ

/*
super.BlockNum
super.FCBNum
super.DataBlockNum
*/

typedef FileControlBlock Inode; // fcb����Inode

// �û�(20B)
typedef struct {
    char user_name[10]; // �û���
    char password[10];  // ����
} User;

// ���ļ���(16B)
typedef struct {
    short inum;         // i�ڵ��
    char file_name[10]; // �ļ���
    short mode;         // ��дģʽ(1:read, 2:write,
                        //         3:read and write)
    short offset;       // ƫ����
} File_table;

char choice;
vector<string> vc_of_str;
string s1, s2;
int inum_cur;                                                                                                                                                   // ��ǰĿ¼
char temp[2 * BLKSIZE];                                                                                                                                         // ������
User user;                                                                                                                                                      // ��ǰ���û�
char bitmap[BLKNUM];                                                                                                                                            // λͼ����
Inode inode_array[INODENUM];                                                                                                                                    // i�ڵ�����
File_table file_array[FILENUM];                                                                                                                                 // ���ļ�������
char image_name[10] = "hd.dat";                                                                                                                                 // �ļ�ϵͳ����
FILE *fp;                                                                                                                                                       // ���ļ�ָ��
const string Commands[] = {"help", "cd", "ls", "mkdir", "touch", "open", "cat", "vi", "close", "rm", "su", "clear", "format", "exit", "rmdir", "info", "copy"}; // 17��

/*����: �û���½����������û��򴴽��û�*/
void login() {
    char *p;
    int flag;
    char user_name[10];
    char password[10];
    char file_name[10] = "user.txt";
    char choice; //ѡ���Ƿ�y/n��
    do {
        printf("login:");
        gets_s(user_name);
        printf("password:");
        p = password;
        while (*p = _getch()) {
            if (*p == 0x0d) //������س���ʱ��0x0dΪ�س�����ASCII��
            {
                *p = '\0'; //������Ļس���ת���ɿո�
                break;
            }
            printf("*"); //�������������"*"����ʾ
            p++;
        }
        flag = 0;
        if ((fp = fopen(file_name, "r+")) == NULL) {
            printf("\nCan't open file %s.\n", file_name);
            printf("This filesystem is not exist now, it will be create~~~\n");
            FormatDisk();
            login();
        }
        while (!feof(fp)) { // feof�ļ�δ��������0
            fread(&user, sizeof(User), 1, fp);
            // �Ѿ����ڵ��û�, ��������ȷ
            if (!strcmp(user.user_name, user_name) && !strcmp(user.password, password)) {
                fclose(fp);
                printf("\n");
                return; //��½�ɹ���ֱ��������½����
            }
            // �Ѿ����ڵ��û�, ���������
            else if (!strcmp(user.user_name, user_name)) {
                printf("\nThis user is exist, but password is incorrect.\n");
                flag = 1; //����flagΪ1����ʾ����������µ�½
                fclose(fp);
                break;
            }
        }
        if (flag == 0) {
            printf("\nThis user is not exist.\n");
            break; //�û������ڣ�������ѭ���������û�
        }
    } while (flag);

    // �������û�
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

//��ʼ��
void init() {
    int i;
    // ��ǰĿ¼Ϊ��Ŀ¼
    inum_cur = 0;
    // ��ʼ�����ļ���
    for (i = 0; i < FILENUM; i++)
        file_array[i].inum = -1;
}

//�����ļ�·�������ڻ���
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

/*����: ��ʾ��������*/
void help() {
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
int analyse() {
    string s = "";
    s1 = "";
    s2 = "";          // s��ȫ�����룻s1���ڴ����s2���ڴ����
    int tabcount = 0; //���ڼ�¼tab���������
    int res = 0;      //������
    while (1) {
        s1 = s; //����Ϊȫ������
        if (s.find(' ') == -1)
            s2 = "";                                                //���벻���ڿո��޲���
        else {                                                      //������ڿո񣬱�ʾ���ڲ���
            while (s1.length() > 0 && s1[s1.length() - 1] == ' ') { // s1ĩβΪ�ո񣬻�δ��ȡ����
                s1 = s1.substr(0, s1.length() - 1);                 //��s1���������ȡ��
            }
            while (s1.length() > 0 && s1[0] == ' ') { //���ʼ�ո�ȥ��
                s1 = s1.substr(1);                    //�ӵ�һ���ַ���ʼ
            }
            if (s1.find(' ') == -1)
                s2 = ""; // s1�޿ո�
            else
                s2 = s1.substr(s1.find_first_of(' ') + 1);          //���������ִ���s2
            while (s2.length() > 0 && s2[s2.length() - 1] == ' ') { //������ĩβ����ո�ȥ��
                s2 = s2.substr(0, s2.length() - 1);
            }
            while (s2.length() > 0 && s2[0] == ' ') { //��������ͷ�ո�ȥ��
                s2 = s2.substr(1);
            }
            s1 = s1.substr(0, s1.find_first_of(' ')); //��whileѭ����ʼ��ȫ������s1������Ϊ����
                                                      //�ڶ��������ǵ�һ���ո��λ�ã�����������
        }
        int ch = _getch(); //�ӿ���̨��ȡһ���ַ���������ʾ����Ļ�ϣ���conio.h��
        if (ch == 8) {     //�˸�
            if (!s.empty()) {
                printf("%c", 8);                 //������ǰһ���ַ���λ��
                printf(" ");                     //�ո�ȡ��
                printf("%c", 8);                 //�ٻ�����ǰһ���ַ���λ�ã����ӽ����Ѻ�
                s = s.substr(0, s.length() - 1); //ȥ�����һ���ַ�
            }
        } else if (ch == 13) { //�س�
            for (res = 0; res < 17; res++) {
                if (s1 == Commands[res])
                    break; //ƥ������
            }
            break;            //��������
        } else if (ch == 9) { // tab

        } else if (ch == ' ') { //�ո���ԣ�����s��
            printf("%c", ch);
            s.push_back(ch);
        } else { //�����ַ����ԣ�����s��
            printf("%c", ch);
            s.push_back(ch);
        }
        //���ڴ�������tab
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
    return res; //����������
}


// ����: �л�Ŀ¼(cd .. ���� cd dir1)
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

// bin/xx ��������bin����
// result_cur ����cd�����ļ��ڵ��
// s2 cd �����·�ɴ�
// inum_cur  ��ǰ�ļ��Ľڵ��
int readby(string path) {	//���ݵ�ǰĿ¼�͵ڶ�������ȷ��ת��ȥ��Ŀ¼
	int result_cur = 0; string s = path;
	if (s.find('/') != -1) {
		s = s.substr(0, s.find_last_of('/') + 1);
	}
	else {
		s = "";
	}
	int temp_cur = inum_cur;
	vector<string> v;
	while (s.find('/') != -1) {  // ��·����ÿһ���ļ��д���vector
		v.push_back(s.substr(0, s.find_first_of('/')));  // ��ȡ��1��б��֮ǰ���ַ���
		s = s.substr(s.find_first_of('/') + 1);  // 
	}
	if (v.size() == 0) { // ˵��û����Ŀ¼��ֱ�ӷ���
		return inum_cur;
	}
	if (v[0].length() == 0) { // û���κ��ƶ��������ڵ�ǰ�ļ���
		temp_cur = 0;
	}
	else if (v[0] == "..") { // ���ص���һ��Ŀ¼
		if (inode_array[inum_cur].Size > 0) {  // ��ǰ�ڵ�Ľڵ�������0
			temp_cur = inode_array[inum_cur].Parent;
		}
	}
	else {
		int i;
		/*
		for (i = 0; i < INODENUM; i++) {
			if ((inode_array[i].inum > 0) &&
				(inode_array[i].iparent == inum_cur) &&  // ��i���ڵ�ĸ��ڵ��ǵ�ǰ�ڵ�
				(inode_array[i].type == 'd') &&
				inode_array[i].file_name == v[0]) {  // ��i���ڵ���ļ�����cd���ĵ�һ���ļ���
				break;  // �ҵ��˵�һ��cd�ļ�
			}
		}
		*/
		i = Find(inum_cur, v[0]);
		if (i == INODENUM) { // ȫ���������ˣ�����û�У���û�и��ļ�·��
			return -1;
		}
		else {
			temp_cur = i;  // �ҵ�
		}
	}
	int i;
	for (unsigned int count = 1; count < v.size(); count++) { // ���ҵ�cd�������ļ���
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

// ����: ��ʾ����
void errcmd()
{
    printf("Command Error!!!\n");
}

// ����: ѭ��ִ���û����������, ֱ��logout
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

// ������
int main(void) {
    // format();
    login();
    init();
    command();
    return 0;
}