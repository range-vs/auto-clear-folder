#include "stdafx.h"
#include <string>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <shellapi.h> // ��� SHFileOperations
#include <algorithm>
#include <thread>
// #include <direct.h>
#include <psapi.h>

#pragma comment( lib, "shell32.lib" ) // ��� SHFileOperation

using namespace std;

#define FATHER_VERSION 0
#define ALL_VERSION 1

string read_config(const string &, vector<string>&); // �������� �������
int* clear_catalogue(string&, ofstream&); // ���������������� ������� ������� ����
	int delete_dir(const char*); // �������� �������� (� �������!)
	HWND GetConsoleHwnd(); // ��������� ����������� ����������� ����
void taimer(ofstream&, bool&, int); // ������, ������� ������� 1 ������, ���� �� ��������� ������ �� ����������� �������� - ��������� ���������
string read_ini(vector<pair< string, int> >&); // ���������� ������� ��� ������ ���������
	bool edit_space(string&); // �������� ��� tab � space �� ���� space
	string read_value(string&, vector<pair< string, int> >&, int); // ������ �� ������ � ��������

	std::string _path;

int main() // name file - path.txt
{
	// ;  - �����������
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);

	int version = ALL_VERSION;

	char current_work_dir[FILENAME_MAX];
	GetModuleFileNameA(NULL, current_work_dir, FILENAME_MAX); // �������� ���������� ������������ �����(hwnd == NULL, ��� ������� �������)
	_path = current_work_dir;
	_path.erase(_path.find_last_of("\\"), (_path.length() - _path.find_last_of("\\"))); // ������� ���� ��������� + "\\"
#ifndef NDEBUG
	std::cout << _path << std::endl;
#endif

	ofstream log(_path + "\\log.txt");
	log.imbue(locale("russian"));
	bool insp(false); // ���������� ������� �� �����������(false)
	vector<pair< string, int> > sections = { { "count_seconds", 0 } };
	vector<string> base;
	string message;

	if ((message = read_ini(sections)) != "") // ������ ������������
	{
		cout << message;
		log << message << "���������� ������ ���������\n";
		log.close();
		system("pause");
		return 0;
	}

	if (version != FATHER_VERSION)
	{
		thread th_time(taimer, ref(log), ref(insp), sections[0].second); // �������� sections[0] ��� ����������� ���-�� ������
		th_time.detach();

		if (MessageBoxA(GetConsoleHwnd(), "������ ��������� ������������ � �������� �������� ���������?", "���������", MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_TOPMOST) == IDNO)
		{
			cout << "������ ������� ������� ���������\n";
			log << "������ ������� ������� ���������\n" << "���������� ������ ���������\n";
			log.close();
			return 0;
		}
		else
			insp = true;
	}

	if ((message = read_config(_path + "\\path.txt", base)) != "")
	{
		cout << message;
		log << message << "���������� ������ ���������\n";
		log.close();
		system("pause");
		return 0;
	}
	if (!base.size())
	{
		cout << "������ ����� ����, ������� ������.\n";
		log << "������ ����� ����, ������� ������.\n" << "���������� ������ ���������\n";
		log.close();
		system("pause");
		return 0;
	}

	cout << "���� ��� ������������: \n";
	log << "���� ��� ������������: \n";
	for (const auto& path : base)
	{
		cout << path << endl;
		log << path << endl;
	}
	cout << endl;
	log << endl;

	for (auto& path : base)
	{
		int* count(nullptr);
		cout << "���� " << endl << path << ":" << endl;
		log << "���� " << endl << path << ":" << endl;
		if ((count= clear_catalogue(path, log)))
		{
			cout << "������� " << count[0] << " ������;\n";
			cout << "������� " << count[1] << " �����;\n";
			cout << "������������: " << count[2] << " ��������.\n\n";

			log << "������� " << count[0] << " ������;\n";
			log << "������� " << count[1] << " �����;\n";
			log << "������������: " << count[2] << " ��������.\n\n";
			delete[]count;
		}
	}

	log << "���������� ������ ���������\n";
	log.close();
	if(version != FATHER_VERSION)
		system("pause");
    return 0;
}

string read_config(const string & name, vector<string>& base)
{
	ifstream file(name);
	if (!file)
	{
		ofstream f(name);
		f.close();
		return "������ �������� �����(path.txt)\n����� � ������ ���� ������� ������.\n";
	}
	while (true)
	{
		string tmp;
		if (!file)
			return "������ ������ �����(path.txt)\n";
		getline(file, tmp, '\n');

		// ������������� ����������� ';' //
		int count(0);
		for (int i(0); i < tmp.length(); i++)
		{
			if (tmp[i] == ' ' || tmp[i] == '\t')
				continue;
			else if (tmp[i] == ';')
			{
				if (count > 0)
				{
					base.push_back(string(tmp.begin(), tmp.begin() + i));
					count = 0;
				}
				break;
			}
			else
				count++;
		}
		if(count > 0)
			base.push_back(tmp);
		//
		if (file.eof())
			break;
	}
	file.close();
	auto _end = remove(base.begin(), base.end(), "");
	base.erase(_end, base.end());

	return "";
}

int* clear_catalogue(string& path, std::ofstream& log)
{
	//extern ofstream log;
	path += "\\*.*";
	WIN32_FIND_DATAA  files_folders_list;
	HANDLE hFind = FindFirstFileA(path.c_str(), &files_folders_list);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		int* count(new int[3]);
		count[0] = 0; // files
		count[1] = 0; // folders
		count[2] = 0; // ignored
		do
		{
			if ((strcmp(files_folders_list.cFileName, ".") != 0) && (strcmp(files_folders_list.cFileName, "..") != 0)) // �� ������� ��������� ��������
			{
				string name = files_folders_list.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "�������: " : "����: ";
				path.resize(path.length() - 4);
				name += path +"\\" + files_folders_list.cFileName; 
				string p(path + "\\" + files_folders_list.cFileName);
				cout << name << endl;
				log << name << endl;

				// ����������, ����� ���������, ��� ����
				if (!(files_folders_list.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // ���� ������ - �� ����
				{
					if (DeleteFileA(p.c_str()))
					{
						cout << "- ���� ������� �����.\n";
						log << "- ���� ������� �����.\n";
						count[0]++;
					}
					else
					{
						int d = GetLastError();
						cout << "+ ���� ����� ������ ���������. �������������.\n";
						log << "+ ���� ����� ������ ���������. �������������.\n"; 
						count[2]++;
					}
				}
				else
				{
					// ����� �����
					char _path[1000];
					strcpy(_path, p.c_str());
					_path[strlen(_path)] = '\0'; // ����������� ��� ���������, ��� ������ - ����� �����
					_path[strlen(_path) + 1] = '\0';

					//DWORD ress = GetLastError();
					if (delete_dir(_path) == 0)
					{
						cout << "- ������� ������� �����.\n";
						log << "- ������� ������� �����.\n";
						count[1]++;
					}
					else
					{
						int d = GetLastError();
						cout << "+ ������� ����� ������ ���������. �������������.\n";
						log << "+ ������� ����� ������ ���������. �������������.\n";
						count[2]++;
					}
				}
				path += "\\*.*";
			}
			/*else // ��������� ��������, �� �����
			{
				cout << "+ ��������� �������: �������������.\n";
				count[2]++;
			}*/
		} while (FindNextFileA(hFind, &files_folders_list));
		FindClose(hFind);
		return count;
	}
	else
	{
		cout << "������ ����: " << path << endl << "� ������� �� ������.\n";
		log << "������ ����: " << path << endl << "� ������� �� ������.\n";
	}
	return nullptr;
}

int delete_dir(const char* path)
{
	SHFILEOPSTRUCTA sh; // ��������� ���� ��� ���������
	sh.hwnd = GetConsoleHwnd(); // ���������� ����
	sh.wFunc = FO_DELETE; // ��������
	sh.pFrom = path; //��������� ����������
	sh.pTo = NULL; // ������� ����������, �� �����
	sh.fFlags = /*FOF_NOCONFIRMATION | FOF_NOERRORUI |*/ FOF_NO_UI; // ������� ���� ��������� ��������, � ����������� �� ����(��,��� � �.�.) + ���� ���� ����� - �� ���������� ��� ��������������
	sh.hNameMappings = 0; // �� �����
	char buf[500]; strcpy(buf, "�������� ����� ");  strcpy(buf, path);
	sh.lpszProgressTitle = buf; // ���, ������������ ��� ������ ��������� ��������
	return SHFileOperationA(&sh); // ������ �������� ��������
}

HWND GetConsoleHwnd() 
{
	HWND hwnd; // ���� ����������
	char Old[200]; // ����� ����� ����
	GetConsoleTitleA(Old, 200); // �������� ������ ��� ����
	SetConsoleTitleA("Console"); // ������ �����
	Sleep(40); // �������� �� ���-�� ���
	hwnd = FindWindowA(NULL, "Console"); // �������� ���������� ������� � ����� ������
	SetConsoleTitleA(Old); // ������ ������ ���
	return hwnd; // ���������� ����������
}

void taimer(ofstream& log, bool& insp, int max_sec)
{
	clock_t _clock(0);
	while (true)
	{
		_clock = clock();
		if ((_clock / CLOCKS_PER_SEC) >= max_sec) // ������ �������������� ������
		{
			//cout << "�����������, ��������� ���������\n";
			log << "�����������, ��������� ���������\n";
			exit(1);
		}
		else if (insp)
			return;
	}
}

string read_ini(vector<pair< string, int> >& sections)
{
	ifstream file(_path + "\\config.ini");
	if (!file)
		return "���������������� ���� �� ���������(config.ini)\n";
	int line(0);
	while (true)
	{
		++line;
		string temp;
		getline(file, temp, '\n');
		if (!file)
			return "������ ������ ����� ������������(config.ini)\n";
		// ������ ��������� ������
		if (edit_space(temp)) // �������� ��� tab � space �� ���� space // false - ������ ��� ������ �����������
		{
			// ���������, ��� ���� ���� ���� ������� � ������ ��� ������, ����� ��������
			string message;
			if ((message = read_value(temp, sections, line)) != "")
			{
				file.close();
				return message;
			}
		}
		if (file.eof())
			break;
	}
	file.close();
	return "";
}

bool edit_space(string& str)
{
	string result;
	while ((str[0] == '\t' || str[0] == ' ') && str.length() > 0) str.erase(0, 1); // ������� ������� � ������ ������
	if (str[0] == ';') // ��� ������ - �����������
		return false;
	for (int i(0); i < str.length(); i++)
	{
		if (str[i] == ';')
			break;
		if (str[i] == '\t' || str[i] == ' ')
		{
			while (i < str.length() && (str[i] == '\t' || str[i] == ' ')) 
				i++;
			if (str[i] == ';')
				break;
			result += ' '; result += str[i];
		}
		else
			result += str[i];
	}
	str = result;
	return true;
}

string read_value(string& str, vector<pair< string, int> >& sections, int line)
{
	int count(0);
	int pos(0);
	while ((pos = str.find(' ', pos+1)) != -1) count++;
	if (count != 1)
		return "������ ������ ����������������� �����(config.ini) � ������ " + to_string(line); // �������� ������
	string section = str.substr(0, str.find(' ')); // ��������� ������, ������ ���� ����� � � ������ ������
	int index(-1);
	for (int i(0); i < sections.size(); i++)
	{
		if (sections[i].first == section)
		{
			index = i;
			break; 
		}
		if (i == sections.size() - 1)
			return "����������� ������ \"" + section + "\" (config.ini)\n";
	}
	str.erase(0, str.find(' ') + 1); // ������� �������� ������
	if (atoi(str.c_str()) != 0) // ������������ �������� � ����� float
		sections[index].second = atoi(str.c_str());
	else
		return "������ � �������� ������ " + sections[index].first + "(config.ini)\n";
	return "";
}
