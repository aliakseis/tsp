// tsp.cpp : Defines the entry point for the console application.
//

#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>

#include <iostream>
#include <string>
#include <vector>
#include <numeric>


using std::cerr;
using std::cout;
using std::string;
using std::vector;


struct Description
{
    double x, y, z; // vector
    int number;
};



#define HANDLE_FLAG(curPlace2) \
    if (flags & (1 << curPlace2)) \
{ \
    const double candidate = curTimes[curPlace2] + connections[curPlace2]; \
    if (candidate < time) \
    time = candidate; \
}

#define HANDLE_GROUPS_4(a, b, c, d) \
    case (a * 64 + b * 16 + c * 4 + d): \
{ \
    enum { flags = (a * 64 + b * 16 + c * 4 + d) }; \
    HANDLE_FLAG(0); \
    HANDLE_FLAG(1); \
    HANDLE_FLAG(2); \
    HANDLE_FLAG(3); \
    HANDLE_FLAG(4); \
    HANDLE_FLAG(5); \
    HANDLE_FLAG(6); \
    HANDLE_FLAG(7); \
    break; \
}

#define HANDLE_GROUPS_3(a, b, c) \
    HANDLE_GROUPS_4(a, b, c, 0) \
    HANDLE_GROUPS_4(a, b, c, 1) \
    HANDLE_GROUPS_4(a, b, c, 2) \
    HANDLE_GROUPS_4(a, b, c, 3)

#define HANDLE_GROUPS_2(a, b) \
    HANDLE_GROUPS_3(a, b, 0) \
    HANDLE_GROUPS_3(a, b, 1) \
    HANDLE_GROUPS_3(a, b, 2) \
    HANDLE_GROUPS_3(a, b, 3)

#define HANDLE_GROUPS_1(a) \
    HANDLE_GROUPS_2(a, 0) \
    HANDLE_GROUPS_2(a, 1) \
    HANDLE_GROUPS_2(a, 2) \
    HANDLE_GROUPS_2(a, 3)


vector<int> GetTime(const int nCount, const int frontDoorIdx, const double (&allConnections)[32][32])
{
    const int nCombs = 1 << nCount;

    double* times = new double[nCount * (nCombs / 2)];

    for (int curComb = 1; curComb < nCombs; ++curComb)
    {
        for (int curPlace = 0, curPlaceBit = 1; curPlaceBit <= curComb; ++curPlace, curPlaceBit <<= 1)
        {
            if (curComb & curPlaceBit)
            {
                int prevComb = curComb ^ curPlaceBit;

                // --------------
                int timesIdx = prevComb;
                if (timesIdx >= nCombs / 2)
                    timesIdx = (nCombs - 1) ^ timesIdx;
                timesIdx *= nCount;

                double time = 1.e10;

                const double *connections = allConnections[curPlace];
                double *curTimes = times + timesIdx;

                for (; prevComb != 0; prevComb >>= 8, connections += 8, curTimes += 8)
                {
                    switch (prevComb & 0xFF)
                    {
                        HANDLE_GROUPS_1(0)
                        HANDLE_GROUPS_1(1)
                        HANDLE_GROUPS_1(2)
                        HANDLE_GROUPS_1(3)
                    }
                }

                if (1.e10 == time)
                    time = 0;
                // --------------

                timesIdx = curComb;
                if (timesIdx >= nCombs / 2)
                    timesIdx = (nCombs - 1) ^ timesIdx;
                timesIdx *= nCount;

                int timeIdx = curPlace + timesIdx;
                times[timeIdx] = time;
            }
        }
    }

    double time = 1.e10;
    int idx = 0;

    for (int i = 0; i < nCount; ++i)
    {
        double candidate = allConnections[frontDoorIdx][i] + times[i];
        if (candidate < time)
        {
            time = candidate;
            idx = i;
        }
    }

    vector<double> connections;
    connections.push_back(allConnections[frontDoorIdx][idx]);
    vector<int> indices;
    indices.push_back(idx);

    int curComb = nCombs - 1;
    while (curComb != 0 && ((curComb & (curComb - 1)) != 0))
    {
        for (int curPlace = 0, curPlaceBit = 1; curPlaceBit <= curComb; ++curPlace, curPlaceBit <<= 1)
        {
            if (curComb & curPlaceBit)
            {
                int prevComb = curComb ^ curPlaceBit;

                int timesIdx = prevComb;
                if (timesIdx >= nCombs / 2)
                    timesIdx = (nCombs - 1) ^ timesIdx;
                timesIdx *= nCount;

                for (int i = 0; i < nCount; ++i)
                {
                    if (std::accumulate(
                            connections.rbegin(), 
                            connections.rend(), 
                            times[timesIdx + i] + allConnections[i][idx])
                        == time)
                    {
                        connections.push_back(allConnections[i][idx]);
                        indices.push_back(i);
                        idx = i;
                        curComb = prevComb;
                        break;
                    }
                }
            }
        }
    }

    delete[] times;

    return indices;
}



int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cerr << "Usage: tsp input_file\n";
		return 1;
	}
/*
	string path(argv[1]);
	if (string::npos == path.find_first_of("\\/"))
	{
		string dir(argv[0]);
		string::size_type pos = dir.find_last_of("\\/");
		if (string::npos != pos)
		{
			path = dir.substr(0, pos + 1) + path;
		}
	}

    FILE* f = fopen(path.c_str(), "r");
//*/
    FILE* f = fopen(argv[1], "r");

    int number;
    double latitude, longitude;

    vector<Description> places;

    Description startDescription;

    while (3 == fscanf(f, "%d%*[^(](%lf,%lf)\n", &number, &latitude, &longitude))
    {
        const double phi = longitude * M_PI / 180.;
        const double theta = latitude * M_PI / 180.;
        
        Description description;

        description.x = cos(theta) * cos(phi);
        description.y = cos(theta) * sin(phi);
        description.z = sin(theta);

        description.number = number;
        if (1 == number)
            startDescription = description;
        else
            places.push_back(description);
    }

    fclose(f);

    const int startIdx = places.size();
    places.push_back(startDescription);

    double allConnections[32][32];

    allConnections[0][0] = 0;
    for (unsigned int i = 1; i < places.size(); ++i)
    {
        allConnections[i][i] = 0;
        for (unsigned int j = 0; j < i; ++j)
        {
            const double distance = acos(
                places[i].x * places[j].x +
                places[i].y * places[j].y +
                places[i].z * places[j].z);
            allConnections[i][j] = allConnections[j][i] = distance;
        }
    }

    vector<int> indices(GetTime(startIdx, startIdx, allConnections));
    cout << startDescription.number << '\n';
    for (vector<int>::iterator it(indices.begin()), itEnd(indices.end()); it != itEnd; ++it)
    {
        cout << places[*it].number << '\n';
    }

	return 0;
}

