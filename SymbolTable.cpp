#include "SymbolTable.h"
#include <algorithm>
using namespace std;

int proc_symbolTable::nexttmpname = 0;

symbolTableItem::symbolTableItem(symbolType type, std::string name, std::string gobalname, int offset)
{
	this->type = type;
	this->name = name;
	this->gobalname = gobalname;
	this->offset = offset;
}

symbolTableItem::symbolTableItem(symbolType type, std::string name, std::string gobalname, int offset, vector<int> array)
{
	this->type = type;
	this->name = name;
	this->gobalname = gobalname;
	this->offset = offset;
	this->array = array;
}

proc_symbolTable::proc_symbolTable() : enter_quad(-1), return_type(symbolType::Unknown), returnAddr(NULL), itemTable_offset(0), type(procSymbolTableType::unknow)
{
}

//创建一个新的函数过程符号表
proc_symbolTable* proc_symbolTable::create_function()
{
	proc_symbolTable* func = new proc_symbolTable();
	func->returnAddr = this;
	return func;
}

//向符号表中插入新变量
void proc_symbolTable::insert_variable(const symbolTableItem& item)
{
	if (this->itemTable.find(item.name) == this->itemTable.end()) //没有重复
	{
		this->itemTable.insert({item.name, item});
		if (item.type == symbolType::Array)
		{
			int offet = symbolTypeOffset.find(item.type)->second;
			for (const auto &i : item.array)offet *= i;
			this->itemTable_offset += offet;
		}
		else
			this->itemTable_offset += symbolTypeOffset.find(item.type)->second;
	}
}

//在符号表中找指定名称的函数
proc_symbolTable* proc_symbolTable::find_function(const std::string& name) const
{
	const proc_symbolTable* gobal = this->find_gobal();
	if (gobal->functionTable.find(name) != gobal->functionTable.end())
		return gobal->functionTable.find(name)->second;
	return NULL;
}

//在符号表中找指定名称的变量
const symbolTableItem& proc_symbolTable::find_variable(std::string name)
{
	proc_symbolTable* tmp = this;
	while (tmp->returnAddr != NULL)
	{
		if (tmp->itemTable.find(name) != tmp->itemTable.end())
			return tmp->itemTable[name];
		tmp = tmp->returnAddr;
	}
	if (tmp->itemTable.find(name) != tmp->itemTable.end())
		return tmp->itemTable[name];
	static symbolTableItem tmpItem;
	tmpItem.type = symbolType::None;
	return tmpItem;
}

//获取全局符号表
const proc_symbolTable* proc_symbolTable::find_gobal() const
{
	const proc_symbolTable* tmp = this;
	while (tmp->returnAddr != NULL)
		tmp = tmp->returnAddr;
	return tmp;

}

//初始化函数过程符号表
void proc_symbolTable::init_function(const std::vector<symbolTableItem>& parms, std::string name, symbolType return_type, int enter_quad)
{
	this->name = name;
	this->return_type = return_type;
	this->parm = parms;
	this->enter_quad = enter_quad;
	this->returnAddr->functionTable.insert({ name, this });
	this->type = procSymbolTableType::function;
	return;
}

//获取上一级符号表
proc_symbolTable* proc_symbolTable::return_block(int enter_quad)
{
	return this->returnAddr;
}

//生成一个新的临时变量名
string proc_symbolTable::newtemp()
{
	string temp = "T";
	temp += std::to_string(proc_symbolTable::nexttmpname);
	proc_symbolTable::nexttmpname++;
	return temp;
}

//返回进入代码块的地址
int proc_symbolTable::get_enterquad() const
{
	return this->enter_quad;
}

//获取偏移量
int proc_symbolTable::get_offset() const
{
	return this->itemTable_offset;
}

//获取所有函数的入口地址及名称
vector<pair<int, string>> proc_symbolTable::getFuncEnter() const
{
	vector<pair<int, string> >ret;
	for (auto iter : this->functionTable)
		ret.push_back(pair<int, string>(iter.second->enter_quad, iter.first));
	sort(ret.begin(), ret.end());
	return ret;
}