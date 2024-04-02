#include "iostream"
#include "Windows.h"
#include "TlHelp32.h"

// DEVELOPED BY Luckm4 from github! //

namespace driver {
	namespace codes {
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}
	struct Request {HANDLE process_id; PVOID target; PVOID buffer; SIZE_T size; SIZE_T return_size;};
	bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
		Request r;
		r.process_id = reinterpret_cast<HANDLE>(pid);
		return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}
	template <class T>
	T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
		T temp = {};
		Request r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = &temp;
		r.size = sizeof(T);
		DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
		return temp;
	}
	template <class T>
	void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& value) {
		Request r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = (PVOID)&value;
		r.size = sizeof(T);
		DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}
}

// Obtem modulos DLL dentro de processos em execução
static DWORD get_module_base(const DWORD pid, const wchar_t* module_name) {
	std::uintptr_t module_base = 0;
	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap_shot == INVALID_HANDLE_VALUE)
		return module_base;
	MODULEENTRY32W entry = {};
	entry.dwSize = sizeof(decltype(entry));
	if (Module32FirstW(snap_shot, &entry) == TRUE) {
		if (wcsstr(module_name, entry.szModule) != nullptr)
			module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
		else {
			while (Module32NextW(snap_shot, &entry) == TRUE) {
				if (wcsstr(module_name, entry.szModule) != nullptr) {
					module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
					break;
				}
			}
		}
	}
	CloseHandle(snap_shot);
	return module_base;
}


// Obtem o ID do processo, serve para o programa identificar o processo pelo ID e não pelo seu nome
static DWORD get_process_id(const char* process_name) {
	DWORD process_id = 0;
	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snap_shot == INVALID_HANDLE_VALUE)
		return process_id;
	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof(entry);

	if (Process32First(snap_shot, &entry) == TRUE) {
		// Verifica se o primeiro handle � o que queremos
		if (_stricmp(process_name, entry.szExeFile) == 0)
			process_id = entry.th32ProcessID;
		else {
			while (Process32Next(snap_shot, &entry) == TRUE) {
				if (_stricmp(process_name, entry.szExeFile) == 0) {
					process_id = entry.th32ProcessID;
					break;
				}
			}
		}
	}
	CloseHandle(snap_shot);
	return process_id;
}



int main()
{
	std::string nomeProcesso = "Processo.exe";
	const DWORD pid = get_process_id(nomeProcesso.c_str());
	const HANDLE driver = CreateFile("\\\\.\\DriverIncrivel", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);

	if (pid == 0) {
		std::cout << "[ - ] Failed to find process\n";
		std::cin.get();
		return 1;
	}

	if (driver == INVALID_HANDLE_VALUE) {
		std::cout << "[ - ] Failed to create our driver handle\n";
		std::cin.get();
		return 1;
	}
	// Função inicial
	if (driver::attach_to_process(driver, pid) == true) {
		std::cout << "Processo identificado e aberto com sucesso!\n";
		std::cout << "[ * ] Driver Pronto";

		// Exemplo de como o driver pode ser utilizado para ler a memoria de um processo!
		DWORD leitura_endereco = driver::read_memory<DWORD>(driver, 0x0000000);

		// Exemplo de como o driver pode ser utilizado para alterar a memoria de um processo!
		driver::write_memory(driver, 0x000000, 1);
	}
	CloseHandle(driver);
	std::cin.get();
	return 0;
}




