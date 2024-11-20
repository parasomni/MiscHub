//Function: algorithm to crack pins with min 4 length
//Compiling: g++ -o pin_cracker pin_cracker.cpp -Wall -Wextra
//last update: 2022

#include <ostream>
#include <Windows.h>
#include <iostream>
#include <string>
#include <time.h>
#include <random>


using namespace std;


namespace console {

	enum color_type
	{
		RED,
		GREEN,
		BLUE,

	};

	namespace priv {

		WORD convert_color_to_foreground(color_type color)
		{
			switch (color)
			{
			case RED:
				return FOREGROUND_RED | FOREGROUND_INTENSITY;
			case GREEN:
				return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			case BLUE:
				return FOREGROUND_BLUE | FOREGROUND_INTENSITY;

			default:
				return 0;
			}
		}

		WORD const FOREGROUND_COLOR_MASK = ~FOREGROUND_BLUE & ~FOREGROUND_RED & ~FOREGROUND_GREEN & ~FOREGROUND_INTENSITY;

		WORD convert_color_to_background(color_type color)
		{
			switch (color)
			{
			case RED:
				return BACKGROUND_RED | BACKGROUND_INTENSITY;
			case GREEN:
				return BACKGROUND_GREEN | BACKGROUND_INTENSITY;
			case BLUE:
				return BACKGROUND_BLUE | BACKGROUND_INTENSITY;
			default:
				return 0;
			}
		}

		WORD const BACKGROUND_COLOR_MASK = ~BACKGROUND_BLUE & ~BACKGROUND_RED & ~BACKGROUND_GREEN & ~BACKGROUND_INTENSITY;

	} // priv

	struct fcolor
	{
	private:
		color_type color_;

	public:
		fcolor(color_type color)
			: color_(color)
		{
		}

		color_type get_color() const { return color_; }
	};

	struct bcolor
	{
	private:
		color_type color_;

	public:
		bcolor(color_type color)
			: color_(color)
		{
		}

		color_type get_color() const { return color_; }
	};

	template<typename CharT, typename TraitsT>
	std::basic_ostream<CharT, TraitsT>& operator <<(std::basic_ostream<CharT, TraitsT>& out, fcolor const& color)
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo = { };
		GetConsoleScreenBufferInfo(handle, &screenBufferInfo);
		screenBufferInfo.wAttributes &= priv::FOREGROUND_COLOR_MASK;
		screenBufferInfo.wAttributes |= priv::convert_color_to_foreground(color.get_color());

		SetConsoleTextAttribute(handle, screenBufferInfo.wAttributes);

		return out;
	}

	template<typename CharT, typename TraitsT>
	std::basic_ostream<CharT, TraitsT>& operator <<(std::basic_ostream<CharT, TraitsT>& out, bcolor const& color)
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo = { };
		GetConsoleScreenBufferInfo(handle, &screenBufferInfo);
		screenBufferInfo.wAttributes &= priv::BACKGROUND_COLOR_MASK;
		screenBufferInfo.wAttributes |= priv::convert_color_to_background(color.get_color());

		SetConsoleTextAttribute(handle, screenBufferInfo.wAttributes);

		return out;
	}

} // console


bool pwdtry(int a, int b, int c, int d, string r_pwd, string user, bool access)
{

	string ax = to_string(a);
	string bx = to_string(b);
	string cx = to_string(c);
	string dx = to_string(d);

	string pwd = ax + bx + cx + dx;

	using namespace console;
	if (pwd == r_pwd)
	{
		cout << fcolor(BLUE);
		cout << "[";
		cout << fcolor(GREEN);
		cout << "+";
		cout << fcolor(BLUE);
		cout << "] ";
		cout << fcolor(GREEN);
		cout << "Access Granted!" << endl;
		cout << fcolor(RED);

		MessageBox(NULL, L"ACCESS GRANTED!", L"Warning! PIN Cracked!", MB_OK);
		cout << fcolor(RED);
		cout << "---------------------" << endl;
		cout << "  Information Data: " << endl;
		cout << "  Username: ";
		cout << fcolor(BLUE);
		cout << user << endl;
		cout << fcolor(RED);
		cout << "  PIN: ";
		cout << fcolor(BLUE);
		cout << pwd << endl;
		cout << fcolor(RED);
		cout << "---------------------" << endl;
		access = true;
	}
	else if (pwd != user)
	{
		cout << fcolor(BLUE);
		cout << "[";
		cout << fcolor(GREEN);
		cout << "-";
		cout << fcolor(BLUE);
		cout << "] Access denied! --|--  ";
		cout << fcolor(RED);
		const int MIN = 0;
		const int MAX = 1;
		constexpr int RAND_NUMS_TO_GENERATE = 10;

		std::random_device rd;
		std::default_random_engine eng(rd());
		std::uniform_real_distribution<float> distr(MIN, MAX);

		for (int n = 0; n < 5; ++n) {

			cout << distr(eng);
		}
		cout << " | PIN try: " << pwd << endl;
		access = false;
	}

	return access;
}



int main()
{
	using namespace console;

	const int min = 4;
	const int max = 15;
	bool access = false;
	char question;
	bool cracked;
	string exit = "xx";
	int a = 0;
	int b = 0;
	int c = 0;
	int d = 0;
	int z_1 = 0;
	int z_2 = 0;
	int z_3 = 0;
	int z_4 = 0;
	int zaehler = 0;
	int zaehler2 = 0;
	int zaehler3 = 0;
	int zaehler4 = 0;



	string Benutzername;
	string Passwort;

	cout << fcolor(RED);

	cout << "  ";
	cout << "  ";
	cout << "___  ___  _  _         ___  ___  ___   ___  _  __ ___  ___" << endl;
	cout << "   | _ \\|_ _|| \\| |       / __|| _ \\/   \\ / __|| |/ /| __|| _ \\ " << endl;
	cout << "   |  _/ | | | .  |      | (__ |   /| - || (__ |   < | _| |   / " << endl;
	cout << "   |_|  |___||_|\\_|       \\___||_|_\\|_|_| \\___||_|\\_\\|___||_|_\\ " << endl;

	cout << "  " << endl;
	cout << " " << endl;
	cout << "powered by parasomni" << endl;
	cout << "version 1.0.1" << endl;
	cout << " " << endl;
	cout << "This is a testtool. " << endl;
	cout << "It shows how long it takes to crack a PIN set by the user." << endl;
	cout << "Let's crack it!" << endl;
	cout << "username is to set by the user" << endl;
	cout << " " << endl;
	cout << " " << endl;

	bool c_length = true;

	while (c_length == true)
	{
		cout << "Username: ";
		cout << fcolor(BLUE);
		cin >> Benutzername;
		cout << fcolor(RED);
		cout << "PIN: ";
		cout << fcolor(BLUE);
		cin >> Passwort;
		cout << fcolor(RED);

		int size = Passwort.size();

		if (size < 4 || size > 4)
		{
			cout << "Incorrect lenght! (4 integers only)" << endl;
			cout << "Try again..." << endl;
			c_length = true;
		}
		else
		{
			c_length = false;
		}
	}

	cout << "Wanna crack it?(j/n)";
	cin >> question;


	
	
	if (question == 'j' || question == 'J')
	{

		while (exit != "e" )
		{
			float x, y;
			clock_t time_req;

			time_req = clock();
			do
			{
				for (; a <= 10 && z_1 < 10; a++, zaehler++)
				{
					if (a == 10)
					{
						a = 0;
					}
					z_1 = 10;
					for (; b <= 10 && z_2 < 10;)
					{
						if (zaehler == 10)
						{
							b++;
							zaehler2++;
							zaehler = 0;
						}
						z_2 = 10;
						if (b == 10)
						{
							b = 0;
						}
						for (; c <= 10 && z_3 < 10;)
						{
							if (zaehler2 == 10)
							{
								c++;
								zaehler3++;
								zaehler2 = 0;
							}
							z_3 = 10;
							if (c == 10)
							{
								c = 0;
							}

							for (; d <= 10 && z_4 < 10; )
							{
								if (zaehler3 == 10)
								{
									zaehler3 = 0;
									d++;
									zaehler4++;
								}
								if (c == 10)
								{
									d = 0;
								}
								if (zaehler4 == 10)
								{
									zaehler4 = 0;
								}
								z_4 = 10;

								cracked = pwdtry(a, b, c, d, Passwort, Benutzername, access);
							}
						}


					}


				}
				z_1 = 0;
				z_2 = 0;
				z_3 = 0;
				z_4 = 0;

			} while (cracked == false);

			time_req = clock() - time_req;
			cout << "Cracked in: ";
			cout << fcolor(BLUE);
			cout << (float)time_req / CLOCKS_PER_SEC << "s." << endl;
			cout << fcolor(RED);
			cout << "press e to exit..." << endl;
			cin >> exit;
		}
	}
	
	else if (question == 'n' || question == 'N')
	{
		cout << "Goodbye..." << endl;
		return 0;
	}
	else
	{
		cout << "Something went wrong..." << endl;
	}



	return 0;
}