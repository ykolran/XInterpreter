#pragma once
#include <vector>
#include <string>

class CLogDataFile
{
public:
	CLogDataFile(int cols, int rows) : numColumns(cols), m_sizeFilled(rows), m_data(new double*[cols])
	{
		for (int i = 0; i < cols; i++)
			m_data[i] = new double[rows];
	}

	~CLogDataFile()
	{
		for (int i = 0; i < numColumns; i++)
			delete []m_data[i];

		delete[] m_data;
	}

	int numColumns;
	std::vector<std::string> columns;

	int m_sizeFilled;
	double ** m_data;


};