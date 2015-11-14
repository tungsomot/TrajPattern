#pragma once
#include <direct.h>
#include <io.h>
#include "GeoPoint.h"
#include "Map.h"
typedef vector<int> MatchedTraj;//��ͼƥ��������

//�������·���Ĺ켣�ļ��е�һ���켣
extern Traj* readOneTrajectory(string &filePath);
//�����ļ��������й켣�ļ�������켣�����ɶ�Ӧ������ļ���
extern void scanTrajFolder(string folderDir, list<Traj*> &trajList, vector<string> &outputFileNames);
//����ָ��·�����ļ������������й켣ƥ�����ļ���������resultList��
extern void readResultFiles(string folderDir, vector<string> &outputFileNames, list<MatchedTraj> &resultList);