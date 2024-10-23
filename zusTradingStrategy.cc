#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <algorithm>

using namespace std;

class Order{ //limit order
public:
	double amount; //expressed in the dollar cost
	double limit;
	bool buy; //if yes, then buy zcn
	int expirationTime;
	Order(double _amount, double _limit, bool _buy, int _expirationTime){
		amount = _amount;
		limit = _limit;
		buy = _buy;
		expirationTime = _expirationTime;
	}
};

class TradingStrategy{
public:
	vector<Order> (*placeOrders)(map<string, vector<double>> data); //place orders based on data
	void backtest(map<string, vector<double>> data); //returns profit
	TradingStrategy(vector<Order> (*func)(map<string, vector<double>>)){
		placeOrders = func;
	}
};

void TradingStrategy::backtest(map<string, vector<double>> data){
	double fiat = 0;
	double zus = 0;
	double volume = 0;
	int nBuys = 0;
	int nSells = 0;
	map<string, vector<double>> dataCapped;
	string arr[4] = {"open", "close", "high", "low"};
	for(string s : arr){
		vector<double> v;
		v.reserve(3000);
		dataCapped.insert(make_pair(s, v));
	}
	int n = data["open"].size();
	for(int i = 0; i < n - 1; i++){
		for(string s : arr){
			dataCapped[s].push_back(data[s][i]);
		}
		vector<Order> order = placeOrders(dataCapped);
		for(int j = 0; j < order.size(); j++){ //execute orders if needed
			for(int k = 1; k <= min(order[j].expirationTime, n - i - 1); k++){
				if(order[j].buy && (data["low"][i + k] <= order[j].limit)){
					fiat -= order[j].amount;
					zus += order[j].amount / order[j].limit;
					volume += order[j].amount;
					nBuys++;
					continue;
				}else if((!order[j].buy) && (data["high"][i + k] >= order[j].limit)){
					fiat += order[j].amount;
					zus -= order[j].amount / order[j].limit;
					volume += order[j].amount;
					nSells++;
					continue;
				}
			}
		}
	}
	cout << "volume: " << volume << endl;
	cout << "buys: " << nBuys << endl;
	cout << "sells: " << nSells << endl;
	cout << "n: " << n << endl;
	cout << "Before final sell: fiat: " << fiat << " zus: " << zus << endl;
	cout << "data close: " << data["close"].back() << endl;

	fiat += zus * data["close"].back();
	zus = 0;

	cout << "fiat after final sell/buy: " << fiat << endl;
}

vector<Order> tradingFunctionTest(map<string, vector<double>> data){
	if(data["open"].size() == 0){
		return vector<Order> ();
	}
	double prevHigh = data["high"].back();
	double prevLow = data["low"].back();
	Order buyOrder(1, prevLow, true, 1);
	Order sellOrder(1, prevHigh, false, 1);
	vector<Order> ret = {buyOrder, sellOrder};
	return ret;
}

map<string, vector<double>> parseData(){
	//map<string, vector<long>> time; //open, close, high, low
	map<string, vector<double>> price; //^ and: volume, marketCap

	ifstream dataText("zusHistoricalData.txt"); //open text file

	if(!dataText.is_open()){
		cerr << "File did not open succesfully";
		return price;
	}

	string firstLine, line, word;
	
	vector<string> columnNames; //column names of dataText

	getline(dataText, firstLine);
	{ //first we create the keywords of map 'price'
		int i = 0;
		int N = 3000;
		stringstream s(firstLine);
		while(getline(s, word, ';')){ //get next word on s
			columnNames.push_back(word); 
			if(4 < i && i < 11){
				vector<double> v; 
				v.reserve(N); //to speed-up future push_backs
				price.insert(make_pair(word, v)); //modify map
			}
			i++;
		}
	}
	while(getline(dataText, line)){
		int j = 0;
		stringstream s(line);
		while(getline(s, word, ';')){
			if(!(4 < j && j < 11)){
				j++;
				continue;
			}
			price[columnNames[j]].push_back(stod(word));
			j++;
		}
	}
	dataText.close();

	{ //reverse vectors
		int i = 0;
		stringstream s(firstLine);
		while(getline(s, word, ';')){ //get next word on s
			columnNames.push_back(word); 
			if(4 < i && i < 11){
				reverse(price[word].begin(), price[word].end());
			}
			i++;
		}
	}

	return price;
}

int main(){
	map<string, vector<double>> price = parseData();
	TradingStrategy tradingStrategy(tradingFunctionTest);
	tradingStrategy.backtest(price);

	return 0;
}