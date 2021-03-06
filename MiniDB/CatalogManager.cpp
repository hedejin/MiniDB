/*
* CatalogManager.cpp
*
*  Created on: 2014年11月4日
*      Author: darren
*/

#include "CatalogManager.h"

CatalogManager::CatalogManager() {
	fCatalog.open(FCATALOG.c_str(), ios::in | ios::out | ios::binary);
	if (!fCatalog.is_open()) {
		cout << "Database File Not Found. Creat Now!" << sizeof(char) << endl;
		init(256);
		fCatalog.open(FCATALOG.c_str(), ios::in | ios::out | ios::binary);
		if (!fCatalog.is_open()) {
			//No exception in construction.
			cout << "Fail to creat database file." << endl;
			return;
		}
	}

}

CatalogManager::~CatalogManager() {
	fCatalog.close();
}

void CatalogManager::init(unsigned numOfBlock) {
	if (!fCatalog.is_open()) {
		ofstream tmp(FCATALOG.c_str());
		char block[BLOCK_SIZE] = { 0 };
		for (unsigned i = 0; i < numOfBlock; i++) {
			tmp.write(block, BLOCK_SIZE);
		}
		tmp.close();
	}
	if (!fCatalog.is_open()) {
		ofstream tmp(FDATA.c_str());
		char block[BLOCK_SIZE] = { 0 };
		for (unsigned i = 0; i < numOfBlock; i++) {
			tmp.write(block, BLOCK_SIZE);
		}
		tmp.close();
	}

}
int CatalogManager::findNextEmptyTable() {
	char buffer[16];
	for (int i = 0;; i++) {
		fCatalog.seekg(FIRST_TABLE_LOCATION + 0x10 * i, ios::beg);
		fCatalog.read(buffer, 16);
		if (fCatalog.eof())
			break;

		if (buffer[0] == 0x0 && buffer[3] == 0) {
			//read会造成偏移
			fCatalog.seekg(FIRST_TABLE_LOCATION + 0x10 * i, ios::beg);
			return FIRST_TABLE_LOCATION + 0x10 * i;
		}
		//else if(buffer[0] == DATA_END)throw(RecordException("No space"));
	}
	return 0;
}
bool CatalogManager::creatTable(string name, vector<Field> fields) {
	if (name.length() > 14)
		throw(CatalogException("Name is too long"));
	if (findTable(name)) {
		cout << name << " Table exist!" << endl;
		return false;
	}
	int locationOfTable = findNextEmptyTable();
	char buffer[16] = { 0 };
	for (unsigned i = 0; i < name.length(); i++) {
		buffer[i] = name[i];
	}
	buffer[name.length()] = 0;
	//<32
	buffer[15] = fields.size();
	fCatalog.write(buffer, 16);
	//将int保存至4位BYTE中
	memcpy(buffer, &locationOfTable, 4);
	int size = 0;
	for (vector<Field>::iterator iter = fields.begin(); iter != fields.end();
	iter++) {
		if (iter->type == INT)
			size += LENGTH_OF_INT;
		else if (iter->type == FLOAT)
			size += LENGTH_OF_FLOAT;
		else if (iter->type == CHAR)
			size += iter->length;
	}
	//字节长度
	memcpy(buffer + 4, &size, 4);
	for (unsigned i = 8; i < 16; i++)
		buffer[i] = 0;
	fCatalog.write(buffer, 16);

	for (vector<Field>::iterator iter = fields.begin(); iter != fields.end();
	iter++) {
		if (iter->name.length() > 11)
			throw(CatalogException("Name is too long"));
		for (unsigned i = 0; i < iter->name.length(); i++) {
			buffer[i] = iter->name[i];
		}
		buffer[iter->name.length()] = 0;
		buffer[12] = 0;//index
		buffer[13] = iter->type;
		buffer[14] = iter->length;
		buffer[15] = iter->attribute;
		fCatalog.write(buffer, 16);
		for (unsigned i = 0; i < 16; i++)
			buffer[i] = 0;
	}
	buffer[0] = DATA_END;
	for (unsigned i = 1; i < 16; i++)
		buffer[i] = 0;
	fCatalog.write(buffer, 16);
	cout << name << " table is created successfully!" << endl;
	return true;
}

void CatalogManager::displayAllTable() {
	fCatalog.seekg(FIRST_TABLE_LOCATION, ios::beg);
	char buffer[16] = { 0 };
	for (;;) {
		fCatalog.read(buffer, 16);
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == 0x0) {
			return;
		}
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DELETE)
			continue;
		string name(buffer);

		cout << " Table " << name << "  NumOfField:"
			<< (unsigned int)(*(unsigned char*)(&buffer[15])) << endl;
		fCatalog.read(buffer, 16);

		for (;;) {
			fCatalog.read(buffer, 16);
			if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DATA_END
				|| (unsigned int)(*(unsigned char*)(&buffer[0]))
				== DELETE)
				break;
			name = string(buffer);
			cout << "		" << name << "\n" << "		" << "type:" << (int)buffer[13]
				<< "\n" << "		" << "length:" << (int)buffer[14] << "\n"
				<< "		" << "attribute:" << (int)buffer[15] << "\n" << endl;
		}
	}
	/*vector<string> names = getAllTable();
	for(vector<string>::iterator iter = names.begin(); iter != names.end(); iter++){
	cout << *iter << endl;
	}*/
}
vector<string> CatalogManager::getAllTable() {
	vector<string> names;
	fCatalog.seekg(FIRST_TABLE_LOCATION, ios::beg);
	char buffer[16] = { 0 };
	for (;;) {
		fCatalog.read(buffer, 16);
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == 0x0) {
			return names;
		}
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DELETE)
			continue;
		names.push_back(string(buffer));

		fCatalog.read(buffer, 16);

		for (;;) {
			fCatalog.read(buffer, 16);
			if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DATA_END
				|| (unsigned int)(*(unsigned char*)(&buffer[0]))
				== DELETE)break;
		}
	}
	return names;
}
bool CatalogManager::findTable(string _name) {
	fCatalog.seekg(FIRST_TABLE_LOCATION, ios::beg);
	char buffer[16] = { 0 };
	for (;;) {
		fCatalog.read(buffer, 16);
		if (buffer[0] == 0x0) {//&& buffer[3] == 0
			return false;
		}
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DELETE)
			continue;
		string name(buffer);
		if (_name == name)
			return true;
		//跳过table内容
		fCatalog.seekg((buffer[15] + 2) * 16, ios::cur);
	}
	return false;
}

Table* CatalogManager::getTableInfo(string _name) {
	fCatalog.seekg(FIRST_TABLE_LOCATION, ios::beg);
	char buffer[16] = { 0 };
	for (;;) {
		fCatalog.read(buffer, 16);
		if (buffer[0] == 0x0) {// && buffer[3] == 0
			return NULL;
		}
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DELETE)
			continue;
		string name(buffer);
		if (_name == name) {

			fCatalog.read(buffer, 16);
			int locationOfTable;
			memcpy(&locationOfTable, buffer, 4);
			int size;
			memcpy(&size, buffer + 4, 4);
			int locationOfData;
			;
			memcpy(&locationOfData, buffer + 8, 4);

			vector<Field> fields;
			for (;;) {
				fCatalog.read(buffer, 16);
				if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DATA_END
					|| (unsigned int)(*(unsigned char*)(&buffer[0]))
					== DELETE)
					break;
				name = string(buffer);
				bool hasIndex = ((unsigned int)(*(unsigned char*)(&buffer[12]))) == 0 ? false : true;
				int type = (unsigned int)(*(unsigned char*)(&buffer[13]));
				int length = (unsigned int)(*(unsigned char*)(&buffer[14]));
				int attribute = (unsigned int)(*(unsigned char*)(&buffer[15]));
				fields.push_back(Field(name, type, attribute, length, hasIndex));
			}
			return new Table(_name, fields, size, locationOfTable,
				locationOfData);

		}
		//跳过table内容
		fCatalog.seekg((buffer[15] + 2) * 16, ios::cur);
	}
	return NULL;
}

bool CatalogManager::dropTable(string _name) {
	fCatalog.seekg(FIRST_TABLE_LOCATION, ios::beg);
	char buffer[16] = { 0 };
	for (;;) {
		fCatalog.read(buffer, 16);
		if (buffer[0] == 0x0) {//&& buffer[3] == 0
			return true;
		}
		if ((unsigned int)(*(unsigned char*)(&buffer[0])) == DELETE)
			continue;
		string name(buffer);

		if (_name == name) {
			//cout << "find " << endl;
			buffer[0] = DELETE;
			int numOfField = (unsigned int)(*(unsigned char*)(&buffer[15]));
			fCatalog.seekg(-16, ios::cur);
			fCatalog.write(buffer, 16);
			for (int i = 0; i < numOfField + 2; i++) {
				fCatalog.write(buffer, 16);
			}
			//跳过table内容
			fCatalog.seekg((numOfField + 2) * 16, ios::cur);
		}
	}
	return false;
}

void CatalogManager::verifyFirstDataLocation(int tableLocation,
	int newLocation) {
	fCatalog.seekg(tableLocation + 0x18, ios::beg);
	fCatalog.write((char*)&newLocation, sizeof(int));
}
bool CatalogManager::deleteAll(string _name) {
	if (findTable(_name))
	{
		Table* table = getTableInfo(_name);
		const int zero = 0;
		fCatalog.seekp(table->locationOfTable + 0x18, ios::beg);
		fCatalog.write((char *)&zero, sizeof(int));
		cout << "Delete successfully! One table affected!" << endl;
		return true;
	}
	else {
		return false;
	}
}
