#include "stdafx.h"
#include <string>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <shellapi.h> // для SHFileOperations
#include <algorithm>
#include <thread>
// #include <direct.h>
#include <psapi.h>

#pragma comment( lib, "shell32.lib" ) // для SHFileOperation

using namespace std;

#define FATHER_VERSION 0
#define ALL_VERSION 1

string read_config(const string &, vector<string>&); // загрузка конфига
int* clear_catalogue(string&, ofstream&); // последовательная очистка каждого пути
	int delete_dir(const char*); // удаление каталога (с ФАЙЛАМИ!)
	HWND GetConsoleHwnd(); // получение дескриптора консольного окна
void taimer(ofstream&, bool&, int); // таймер, который считает 1 минуту, если по истечении минуты не предпринято действий - закрываем программу
string read_ini(vector<pair< string, int> >&); // считывание конфига для работы программы
	bool edit_space(string&); // заменяем все tab и space на один space
	string read_value(string&, vector<pair< string, int> >&, int); // парсим на секцию и значение

	std::string _path;

int main() // name file - path.txt
{
	// ;  - комментарий
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);

	int version = ALL_VERSION;

	char current_work_dir[FILENAME_MAX];
	GetModuleFileNameA(NULL, current_work_dir, FILENAME_MAX); // получаем директорию исполняемого файла(hwnd == NULL, это текущий процесс)
	_path = current_work_dir;
	_path.erase(_path.find_last_of("\\"), (_path.length() - _path.find_last_of("\\"))); // удаляем имся экзешника + "\\"
#ifndef NDEBUG
	std::cout << _path << std::endl;
#endif

	ofstream log(_path + "\\log.txt");
	log.imbue(locale("russian"));
	bool insp(false); // изначально выбрано не сканировать(false)
	vector<pair< string, int> > sections = { { "count_seconds", 0 } };
	vector<string> base;
	string message;

	if ((message = read_ini(sections)) != "") // грузим конфигурацию
	{
		cout << message;
		log << message << "Завершение работы программы\n";
		log.close();
		system("pause");
		return 0;
	}

	if (version != FATHER_VERSION)
	{
		thread th_time(taimer, ref(log), ref(insp), sections[0].second); // передать sections[0] для ограничения кол-ва секунд
		th_time.detach();

		if (MessageBoxA(GetConsoleHwnd(), "Хотите запустить сканирование и удаление заданных каталогов?", "Сообщение", MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_TOPMOST) == IDNO)
		{
			cout << "Отмена запуска очистки каталогов\n";
			log << "Отмена запуска очистки каталогов\n" << "Завершение работы программы\n";
			log.close();
			return 0;
		}
		else
			insp = true;
	}

	if ((message = read_config(_path + "\\path.txt", base)) != "")
	{
		cout << message;
		log << message << "Завершение работы программы\n";
		log.close();
		system("pause");
		return 0;
	}
	if (!base.size())
	{
		cout << "Список путей пуст, очищать нечего.\n";
		log << "Список путей пуст, очищать нечего.\n" << "Завершение работы программы\n";
		log.close();
		system("pause");
		return 0;
	}

	cout << "Пути для сканирования: \n";
	log << "Пути для сканирования: \n";
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
		cout << "Путь " << endl << path << ":" << endl;
		log << "Путь " << endl << path << ":" << endl;
		if ((count= clear_catalogue(path, log)))
		{
			cout << "Удалено " << count[0] << " файлов;\n";
			cout << "Удалено " << count[1] << " папок;\n";
			cout << "Игнорировано: " << count[2] << " объектов.\n\n";

			log << "Удалено " << count[0] << " файлов;\n";
			log << "Удалено " << count[1] << " папок;\n";
			log << "Игнорировано: " << count[2] << " объектов.\n\n";
			delete[]count;
		}
	}

	log << "Завершение работы программы\n";
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
		return "Ошибка открытия файла(path.txt)\nНовый и чистый файл успешно создан.\n";
	}
	while (true)
	{
		string tmp;
		if (!file)
			return "Ошибка чтения файла(path.txt)\n";
		getline(file, tmp, '\n');

		// распознавание комментария ';' //
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
			if ((strcmp(files_folders_list.cFileName, ".") != 0) && (strcmp(files_folders_list.cFileName, "..") != 0)) // не выводим системные каталоги
			{
				string name = files_folders_list.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "Каталог: " : "Файл: ";
				path.resize(path.length() - 4);
				name += path +"\\" + files_folders_list.cFileName; 
				string p(path + "\\" + files_folders_list.cFileName);
				cout << name << endl;
				log << name << endl;

				// определяем, папку встретили, или файл
				if (!(files_folders_list.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // если истина - то файл
				{
					if (DeleteFileA(p.c_str()))
					{
						cout << "- файл успешно удалён.\n";
						log << "- файл успешно удалён.\n";
						count[0]++;
					}
					else
					{
						int d = GetLastError();
						cout << "+ файл занят другим процессом. Игнорирование.\n";
						log << "+ файл занят другим процессом. Игнорирование.\n"; 
						count[2]++;
					}
				}
				else
				{
					// иначе папка
					char _path[1000];
					strcpy(_path, p.c_str());
					_path[strlen(_path)] = '\0'; // обязательно два терминала, ибо строка - набор путей
					_path[strlen(_path) + 1] = '\0';

					//DWORD ress = GetLastError();
					if (delete_dir(_path) == 0)
					{
						cout << "- каталог успешно удалён.\n";
						log << "- каталог успешно удалён.\n";
						count[1]++;
					}
					else
					{
						int d = GetLastError();
						cout << "+ каталог занят другим процессом. Игнорирование.\n";
						log << "+ каталог занят другим процессом. Игнорирование.\n";
						count[2]++;
					}
				}
				path += "\\*.*";
			}
			/*else // системные каталоги, не нужно
			{
				cout << "+ системный каталог: Игнорирование.\n";
				count[2]++;
			}*/
		} while (FindNextFileA(hFind, &files_folders_list));
		FindClose(hFind);
		return count;
	}
	else
	{
		cout << "Данный путь: " << path << endl << "в системе не найден.\n";
		log << "Данный путь: " << path << endl << "в системе не найден.\n";
	}
	return nullptr;
}

int delete_dir(const char* path)
{
	SHFILEOPSTRUCTA sh; // структура пути для обработки
	sh.hwnd = GetConsoleHwnd(); // дескриптор окна
	sh.wFunc = FO_DELETE; // операция
	sh.pFrom = path; //удаляемая директория
	sh.pTo = NULL; // каталог назначения, не нужен
	sh.fFlags = /*FOF_NOCONFIRMATION | FOF_NOERRORUI |*/ FOF_NO_UI; // выводим окно прогресса удаления, и соглашаемся со всем(да,нет и т.д.) + если файл занят - то игнорируем без предупреждений
	sh.hNameMappings = 0; // не нужно
	char buf[500]; strcpy(buf, "Удаление папки ");  strcpy(buf, path);
	sh.lpszProgressTitle = buf; // имя, отображаемое при выводе прогресса удаления
	return SHFileOperationA(&sh); // запуск операции удаления
}

HWND GetConsoleHwnd() 
{
	HWND hwnd; // сама переменная
	char Old[200]; // буфер имени окна
	GetConsoleTitleA(Old, 200); // получаем старое имя окна
	SetConsoleTitleA("Console"); // задаем новое
	Sleep(40); // засыпаем на кой-то ляд
	hwnd = FindWindowA(NULL, "Console"); // получаем дескриптор консоли с новым именем
	SetConsoleTitleA(Old); // задаем старое имя
	return hwnd; // возвращаем дескриптор
}

void taimer(ofstream& log, bool& insp, int max_sec)
{
	clock_t _clock(0);
	while (true)
	{
		_clock = clock();
		if ((_clock / CLOCKS_PER_SEC) >= max_sec) // прошла приблизительно минута
		{
			//cout << "Бездействие, программа завершена\n";
			log << "Бездействие, программа завершена\n";
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
		return "Конфигурационный файл не обнаружен(config.ini)\n";
	int line(0);
	while (true)
	{
		++line;
		string temp;
		getline(file, temp, '\n');
		if (!file)
			return "Ошибка чтения файла конфигурации(config.ini)\n";
		// теперь распарсим строку
		if (edit_space(temp)) // заменить все tab и space на один space // false - значит вся строка комментарий
		{
			// проверяем, что есть лишь один проблел и читаем имя секции, затем значение
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
	while ((str[0] == '\t' || str[0] == ' ') && str.length() > 0) str.erase(0, 1); // удаляем пробелы в начале строки
	if (str[0] == ';') // вся строка - комментарий
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
		return "Ошибка чтения конфигурационного файла(config.ini) в строке " + to_string(line); // добавить строку
	string section = str.substr(0, str.find(' ')); // извлекаем секцию, теперь надо найти её в списке секций
	int index(-1);
	for (int i(0); i < sections.size(); i++)
	{
		if (sections[i].first == section)
		{
			index = i;
			break; 
		}
		if (i == sections.size() - 1)
			return "Неизвестная секция \"" + section + "\" (config.ini)\n";
	}
	str.erase(0, str.find(' ') + 1); // удаляем название секции
	if (atoi(str.c_str()) != 0) // конвертируем значение в число float
		sections[index].second = atoi(str.c_str());
	else
		return "Ошибка в значении секции " + sections[index].first + "(config.ini)\n";
	return "";
}
