#include<iostream>
#include<iterator>
#include<string>
#include <time.h>
#include"../MapLibraries/Map.h"
using namespace std;

string rootDirectory = "D:\\Document\\MDM Lab\\Data\\";
string mapDirectory = "新加坡轨迹数据\\";
string poiFilePath = "NDBC扩展\\poi.csv";
string semanticRoadFilePath = "semanticRoad.txt";
double neighborRange = 200.0;
int KMEANS_K = 13;
Map routeNetwork(rootDirectory + mapDirectory, 500);
map<string, int> categories;
int poiSize = 13;
//读入POI数据，依此填充路段的poiNums数组
//对于每个POI点，寻找neighborRange范围内的路段，在路段的poiNums数组中对应POI种类计数加1
void generateSemanticRouteNetwork() {
	ifstream fin(rootDirectory + poiFilePath);
	double lat, lon;
	string category;
	char separator;
	fin >> category;
	int last = 0;
	int categoryIndex = 0;
	for (int i = 0; i < category.size(); ++i) {
		if (category[i] == ',') {
			categories.insert(make_pair(category.substr(last, i - last), categoryIndex++));
			last = i + 1;
		}
	}
	categories.insert(make_pair(category.substr(last, category.size() - last), categoryIndex++));
	while (fin >> lat >> separator >> lon >> separator >> category) {
		vector<Edge*> dest;
		routeNetwork.getNearEdges(lat, lon, neighborRange, dest);
		for each (Edge* edgePtr in dest)
		{
			if (edgePtr->poiNums.size() == 0) {
				edgePtr->poiNums = vector<double>(categories.size());
			}
			++edgePtr->poiNums[categories[category]];
		}
	}
	fin.close();
}

//对每个路段的poiNums数组进行归一化处理，每个POI种类数量除以该路段附近总的POI数量
void poiNumsNormalize() {
	for each (Edge* edge in routeNetwork.edges)
	{
		if (edge == NULL) continue;
		int count = 0;
		for each(double num in edge->poiNums) {
			count += static_cast<int>(num);
		}
		if (count == 0) continue;
		for (int i = 0; i < edge->poiNums.size(); ++i) {
			edge->poiNums[i] = edge->poiNums[i] / count;
		}
	}
}


//输出路段的poiNums数组至指定文件
//文件格式为：
//第一行：		POI种类名称（逗号分隔）
//第二行及以后：	路段Id,第一种POI的归一化数量,第二种POI的归一化数量,……最后一种POI的归一化数量,路段所属语义聚类Id（即globalSemanticType）
void outputSemanticRouteNetwork(string filePath) {
	ofstream fout(filePath);
	bool first = true;
	for each (pair<string, int> category in categories)
	{
		if (first) {
			first = false;
		}
		else {
			fout << ",";
		}
		fout << category.first;
	}
	fout << endl;
	for each(Edge* edgePtr in routeNetwork.edges) {
		if (edgePtr == NULL) continue;
		fout << edgePtr->id;
		for each(double num in edgePtr->poiNums) {
			fout << "," << num;
		}
		fout << "," << edgePtr->globalSemanticType<<endl;
	}
	fout.close();
}
//求距离
double getDistance(Edge* edge1, Edge* edge2)
{
	double t1 = 0, t2 = 0, t3 = 0;
	for (int i = 0; i < poiSize; i++)
	{
		t1 += edge1->poiNums[i] * edge2->poiNums[i];
		t2 += edge1->poiNums[i] * edge1->poiNums[i];
		t3 += edge2->poiNums[i] * edge2->poiNums[i];
	}
	return 1 - t1 / sqrt(t2) / sqrt(t3);
}

Edge* calcCenter(vector<Edge*>& edges)
{
	Edge* center = new Edge();
	center->poiNums = vector<double>(poiSize, 0);
	for (int i = 0; i < poiSize; i++)
	{
		for (auto edge : edges)
			center->poiNums[i] += edge->poiNums[i];
		center->poiNums[i] /= edges.size();
	}
	double t = 0;
	for (int i = 0; i < poiSize; i++)
		t += center->poiNums[i] * center->poiNums[i];
	t = sqrt(t);
	for (int i = 0; i < poiSize; i++)
		center->poiNums[i] /= t;
	return center;
}

double calcSSE(semanticCluster& cluster, Edge*center)
{

	double SSE = 0;
	for (auto edge : cluster.cluster)
	{
		SSE += getDistance(edge, center);
	}
	cluster.SSE = SSE;
	return SSE;
}

void splitCluster(vector<semanticCluster>&clusters, int maxj)
{
	int iterTime = 15, testTime = 20, mj;
	double minSSE = 1e10, SSE;
	vector<semanticCluster>a(testTime), b(testTime);
	Edge*center1, *center2;
	srand(unsigned(time(NULL)));
	for (int i = 0; i < testTime; i++)
	{
		int t1 = rand() % clusters[maxj].cluster.size(), t2 = rand() % clusters[maxj].cluster.size();
		while (t1 == t2 || getDistance(clusters[maxj].cluster[t1], clusters[maxj].cluster[t2])<1e-10)t2 = rand() % clusters[maxj].cluster.size();
		center1 = clusters[maxj].cluster[t1]; center2 = clusters[maxj].cluster[t2];
		for (int j = 0; j < iterTime; j++)
		{
			a[i].cluster.clear(); b[i].cluster.clear();
			for (int k = 0; k < clusters[maxj].cluster.size(); k++)
				if (getDistance(clusters[maxj].cluster[k], center1) < getDistance(clusters[maxj].cluster[k], center2))
					a[i].cluster.push_back(clusters[maxj].cluster[k]);
				else b[i].cluster.push_back(clusters[maxj].cluster[k]);
				center1 = calcCenter(a[i].cluster); center2 = calcCenter(b[i].cluster);
		}
		SSE = calcSSE(a[i], center1) + calcSSE(b[i], center2);
		if (SSE < minSSE) { minSSE = SSE; mj = i; }
	}
	clusters[maxj] = a[mj]; clusters.push_back(b[mj]);
}


//计算路段所属种类
void getGlobalSemanticType(vector<Edge*> &edges, int k)
{
	vector<semanticCluster>clusters;
	semanticCluster cluster;
	for each (Edge* edge in edges)
		if (edge&&edge->poiNums.size() >= poiSize)
			cluster.cluster.push_back(edge);
	clusters.push_back(cluster);
	double  maxSSE; int maxj = 0;
	for (int i = 1; i < k; i++)
	{
		maxSSE = 0;
		for (int j = 0; j < clusters.size(); j++)
			if (clusters[j].SSE>maxSSE)
			{
				maxSSE = clusters[j].SSE; maxj = j;
			}
		splitCluster(clusters, maxj);

	}
	for (int i = 1; i <= k; i++)
		for (auto edge : clusters[i - 1].cluster)
			edge->globalSemanticType = i;
}
void main() {
	generateSemanticRouteNetwork();
	poiNumsNormalize();
	getGlobalSemanticType(routeNetwork.edges, KMEANS_K);
	outputSemanticRouteNetwork(semanticRoadFilePath);
}