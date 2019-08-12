// GenerateAst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
#include <algorithm>

using namespace std;

void defineType(
	ofstream& writer, string baseName,
	string structName, string fieldList) 
{
	writer << "struct " << structName << " : " <<	baseName << "\n";
	writer << "{\n";

	istringstream iss(fieldList);
	vector<string> fields((istream_iterator<string>(iss)),
		istream_iterator<string>());

	// Constructor.                                              
	writer << "    " << structName << "(";
	for (size_t i = 0; i < fields.size(); ++i)
	{
		if (i % 2 == 0)
		{
			// type name
			if (fields[i] == "Expr")
				writer << "std::unique_ptr<Expr>&& ";
			else if (fields[i] == "std::vector<std::unique_ptr<Stmt>>")
				writer << fields[i] << "&& ";
			else
				writer << fields[i] << " ";
		}
		else // variable name
		{
			writer << fields[i];
			if (i != fields.size() - 1)
				writer << ",";
		}
	}

	// Store parameters in fields.                               
	writer << ") : \n";
	for (size_t i = 1; i < fields.size(); i+=2)
	{
		writer << "        m_" << fields[i] << "(";
		if (fields[i-1] == "Expr" ||
			fields[i - 1] == "std::vector<std::unique_ptr<Stmt>>")
			writer << "move(" << fields[i] << "))";
		else
			writer << fields[i] << ")";
		if (i != fields.size() - 1)
			writer << ",";
		writer << "\n";
	}
	writer << "        { }\n\n";

	writer << "    virtual void accept(" << baseName << "Visitor& visitor) const override\n";
	writer << "    {\n";
	writer << "        return visitor.visit" <<
		structName << baseName << "(*this);\n";
	writer << "    }\n\n";

	// Fields.                                                   
	for (size_t i = 0; i < fields.size(); ++i)
	{
		if (i % 2 == 0) // type name
		{
			if (fields[i] == "Expr")
				writer << "    std::unique_ptr<" << fields[i] << "> ";
			else
				writer << "    " << fields[i] << " ";
		}
		else // variable name
		{
			writer << "m_" << fields[i] << ";\n";
		}
	}

	writer << "};\n\n";
}

void defineVisitor(ofstream& writer, string baseName, const vector<string>& types) 
{
	writer << "struct " << baseName << "Visitor\n{\n";
	string lowerCaseBaseName = baseName;
	transform(lowerCaseBaseName.begin(), lowerCaseBaseName.end(), lowerCaseBaseName.begin(), ::tolower);

	for (string type : types) {
		size_t ind = type.find(':');
		string structName = type.substr(0, ind);
		writer << "    virtual void visit" << structName << baseName << "(const " <<
			structName << "& " << lowerCaseBaseName << ") = 0;\n";
	}

	writer << "};\n\n";
}

void defineAst(string outputDir, string baseName, const vector<string>& types)
{
	string path = outputDir + "/" + baseName + ".h";
	ofstream writer(path);

	writer << "#pragma once\n";
	writer << "#include \"Token.h\"\n";
	writer << "#include <memory>\n";
	writer << "#include <string>\n";
	if (baseName != "Expr")
		writer << "#include \"Expr.h\"\n";
	writer << "\n";

	// FW declarations
	writer << "// FW declarations\n";
	writer << "struct " << baseName << "Visitor;\n";
	for (string type : types) {
		size_t ind = type.find(':');
		string structName = type.substr(0, ind);
		writer << "struct " << structName << ";\n";
	}
	writer << "\n";

	writer << "struct " << baseName << "\n";
	writer << "{\n";
	writer << "    virtual void accept(" << baseName << "Visitor& visitor) const = 0;\n";
	writer << "    virtual ~" << baseName << "() { }\n";
	writer << "};\n";
	writer << "\n";

	defineVisitor(writer, baseName, types);

	for (string type : types) {
		size_t ind = type.find(':');
		string structName = type.substr(0, ind);
		ind = type.find(':', ind);
		string fields = type.substr(ind + 1);
		defineType(writer, baseName, structName, fields);
	}

	writer.close();
}


int main()
{
	string outputDir = "../XInterpreter";
	defineAst(outputDir, "Expr", vector<string>{
			"Assign:Token name Expr value",
			"Binary:Expr left Token op Expr right",
			"Grouping:Expr expression",
			"Literal:double value",
			"String:std::string value",
			"Unary:Token op Expr right",
			"Variable:Token name"
	});

	defineAst(outputDir, "Stmt", vector<string>{
			"Block:std::vector<std::unique_ptr<Stmt>> statements",
			"Expression:Expr expression",
			"Print:Expr expression",
			"Var:Token name Expr initializer"
	});


    return 0;
}

