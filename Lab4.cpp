#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <bitset>
#include <queue>
#include <stack>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

static const int NUM_OF_ARMS = 4;

// Classes Prototypes
// Read_File Class
// Reads the RobotData.xml file and collects the raw data
class Read_File {
private:
	ifstream fin;
	string data;
public:
	Read_File() {};
	void readFile(string file_name);
	string getData();
};

// XMLParser Class
// Parses and sorts the data into a collection of bitsets
class XMLParser {
private:
	string data;
	queue <bitset <18>> dataCollection;
	void organizeQueueData(queue <string>& q);
public:
	XMLParser() : data("N/A") {};
	void setData(string d);
	void parse();
	queue <bitset <18>> getDataCollection();
};

// Command Class
// Holds the specific data found inside the bitsets
class Command {
private:
	int motor_number;
	string speed;
	string orientation;
	string direction;
	int time;
public:
	Command() : motor_number(-1), speed("N/A"), orientation("N/A"), direction("N/A"), time(0) {};
	void setMotor_Number(int mn);
	void setSpeed(string s);
	void setOrientation(string o);
	void setDirection(string d);
	void setTime(int t);
	int getMotor_Number();
	string getSpeed();
	string getOrientation();
	string getDirection();
	int getTime();
	void printCommand();
};

// CommandQueue Class
// Holds and sorts all the bitset data translated into "Commands"
// based on the arm it is meant for
class CommandQueue {
private:
	vector <queue <Command>> commands;
	bool readyToSend;
	condition_variable cv;
	mutex lookup_lock;
public:
	CommandQueue() : readyToSend(false) { commands.resize(NUM_OF_ARMS); };
	Command& getFront(int mn);
	void insert(Command& c);
	void pop(int mn);
	bool isEmpty(int mn);
	bool& accessReadyToSend();
	condition_variable& accessCV();
};

// Processor Class
// Reads, parses and holds all data found in the RobotArm.xml file
class Processor {
private:
	Read_File* reader;
	XMLParser* parser;
	queue <bitset <18>> rawCommands;
	CommandQueue* commandList;
	mutex mtx1;
	map <int, string> directionCipher;
	map <int, string> speedCipher;
	int findMotorNum(bitset <18>& c);
	string findSpeed(bitset <18>& c);
	string findDirection(bitset <18>& c);
	int findTime(bitset <18>& c);
public:
	Processor();
	~Processor();
	void readFile(string file_name);
	void parseFile();
	void decodeOpcode();
	CommandQueue* getCommandList();
};

// Arm Class
// Holds a single thread and act as actions for a specific arm
class Arm {
private:
	int arm_number;
	thread arm_action;
public:
	Arm() : arm_number(-1) {};
	Arm(const Arm& a) : arm_number(a.arm_number), arm_action(thread()) {};
	~Arm();
	void setArmNumber(int an);
	thread& accessArm();
	int getArmNumber();
};

// Motor Class
// Controls all the arms that depend on itself
class Motor {
private:
	vector <Arm> arms;
	CommandQueue* commandList;
	mutex beforeStart;
	mutex motorActionLock;
public:
	Motor();
	~Motor();
	void setCommandList(CommandQueue* cL);
	void start();
	void execute(int arm_number);
};

// ControlPanel Class
// Controls the entire process of reading, parsing, organizing and 
// execute the commands from the RobotData.xml file
class ControlPanel {
private:
	Processor central_processor;
	Motor main_motor;
public:
	ControlPanel() {};
	void startProgram(string file_name);
};

// Main Function
int main()
{
	ControlPanel main_panel;
	main_panel.startProgram("c:/Users/Vincent Ha/Downloads/RobotData.xml");
    return 0;
}

// Function Definitions
// Read_File Function Definitions
void Read_File::readFile(string file_name) {
	string name, content;
	string entireFile = "";
	fin.open(file_name);
	if (!fin.good()) {
		cerr << "Invalid File Path. Please Reload With Proper File Path." << endl;
		exit(-92);
	}

	cout << "Reading data from the file." << endl;
	while (!fin.eof()) {
		getline(fin, content);
		entireFile += content;
		entireFile += "\n";
		content.clear();
	}
	fin.close();
	data = entireFile;
}

string Read_File::getData() {
	return data;
}

// XMLParser Function Definitions
void XMLParser::organizeQueueData(queue <string>& q) {
	while (!q.empty()) {
		bitset <18> temp(q.front());
		q.pop();
		dataCollection.push(temp);
	}
}

void XMLParser::setData(string d) {
	data = d;
}

void XMLParser::parse() {
	cout << "Processing the file." << endl;
	regex pattern4("(1|0){2,}|<\\w+>");
	regex pattern5("<\\w+>");
	regex pattern6("\/|\t");
	data = regex_replace(data, pattern6, "");

	sregex_token_iterator iter(data.begin(), data.end(), pattern4);
	stack <string> tags;
	queue <string> info;
	string temp;

	tags.push(static_cast<string>(*iter));
	iter++;
	while (!tags.empty()) {
		temp = static_cast<string>(*iter);
		if (regex_match(temp, pattern5)) {
			if (tags.top() == temp)
				tags.pop();
			else
				tags.push(temp);
		}
		else
			info.push(temp);
		iter++;
	}
	organizeQueueData(info);
}

queue <bitset <18>> XMLParser::getDataCollection() {
	return dataCollection;
}

// Command Function Defintions
void Command::setMotor_Number(int mn) {
	motor_number = mn;
}

void Command::setSpeed(string s) {
	speed = s;
}

void Command::setOrientation(string o) {
	orientation = o;
}

void Command::setDirection(string d) {
	direction = d;
}

void Command::setTime(int t) {
	time = t;
}

int Command::getMotor_Number() {
	return motor_number;
}

string Command::getSpeed() {
	return speed;
}

string Command::getOrientation() {
	return orientation;
}

string Command::getDirection() {
	return direction;
}

int Command::getTime() {
	return time;
}

void Command::printCommand() {
	cout << "Robot Arm Number: " << motor_number + 1 << endl;
	cout << "Speed: " << speed << endl;
	cout << "Orientation: " << orientation << endl;
	cout << "Direction: " << direction << endl;
	cout << "Time: " << time << " ms" << endl;
	cout << endl;
}

// CommandQueue Function Definitions
Command& CommandQueue::getFront(int mn) {
	unique_lock <mutex> lock(lookup_lock);
	return commands[mn].front();
}

void CommandQueue::insert(Command& c) {
	unique_lock <mutex> lock(lookup_lock);
	commands[c.getMotor_Number()].push(c);
}

void CommandQueue::pop(int mn) {
	unique_lock <mutex> lock(lookup_lock);
	commands[mn].pop();
}

bool CommandQueue::isEmpty(int mn) {
	unique_lock <mutex> lock(lookup_lock);
	return commands[mn].empty();
}

bool& CommandQueue::accessReadyToSend() {
	return readyToSend;
}

condition_variable& CommandQueue::accessCV() {
	return cv;
}

// Processor Function Definitions
int Processor::findMotorNum(bitset <18>& c) {
	bitset <18> mask("110000000000000000");
	bitset <18> tempBitset = (c & mask) >> 16;
	return tempBitset.to_ulong();
}

string Processor::findSpeed(bitset <18>& c) {
	bitset <18> mask("001000000000000000");
	bitset <18> tempBitset = (c & mask) >> 15;
	return speedCipher[tempBitset.to_ulong()];
}

string Processor::findDirection(bitset <18>& c) {
	bitset <18> mask("000110000000000000");
	bitset <18> tempBitset = (c & mask) >> 13;
	return directionCipher[tempBitset.to_ulong()];
}

int Processor::findTime(bitset <18>& c) {
	bitset <18> mask("000001111111111111");
	bitset <18> tempBitset = c & mask;
	return tempBitset.to_ulong();
}

Processor::Processor() : reader(new Read_File), parser(new XMLParser), commandList(new CommandQueue) {
	directionCipher.emplace(0, "Horizontal Left");
	directionCipher.emplace(1, "Horizontal Right");
	directionCipher.emplace(2, "Vertical Down");
	directionCipher.emplace(3, "Vertical Up");
	speedCipher.emplace(0, "Slow");
	speedCipher.emplace(1, "Fast");
}

Processor::~Processor() {
	delete reader;
	delete parser;
	delete commandList;
}

void Processor::readFile(string file_name) {
	reader->readFile(file_name);
	parser->setData(reader->getData());
}

void Processor::parseFile() {
	parser->parse();
	rawCommands = parser->getDataCollection();
}

void Processor::decodeOpcode() {
	Command tempCommand;
	queue <bitset<18>> copy = rawCommands;
	bitset <18> tempBitset;
	string direction;
	regex noSpaces("[^ ]+");
	sregex_token_iterator iter;

	{
		unique_lock <mutex> lock(mtx1);
		while (!copy.empty()) {
			tempBitset = copy.front();
			copy.pop();
			tempCommand.setMotor_Number(findMotorNum(tempBitset));
			tempCommand.setSpeed(findSpeed(tempBitset));
			tempCommand.setTime(findTime(tempBitset));
			direction = findDirection(tempBitset);
			iter = sregex_token_iterator(direction.begin(), direction.end(), noSpaces);
			tempCommand.setOrientation(static_cast<string>(*iter));
			iter++;
			tempCommand.setDirection(static_cast<string>(*iter));
			commandList->insert(tempCommand);
		}

		cout << "Implementing Commands" << endl;
		cout << endl;
		cout << "Commands" << endl;
		cout << "--------" << endl;
		commandList->accessReadyToSend() = true;
		commandList->accessCV().notify_all();
	}
}

CommandQueue* Processor::getCommandList() {
	return commandList;
}

// Arm Function Definitions
Arm::~Arm() {
	if(arm_action.joinable())
		arm_action.join();
}

void Arm::setArmNumber(int an) {
	arm_number = an;
}

thread& Arm::accessArm() {
	return arm_action;
}

int Arm::getArmNumber() {
	return arm_number;
}

// Motor Function Definitions
Motor::Motor() : commandList(NULL) {
	for (int armCount = 0; armCount < NUM_OF_ARMS; armCount++) {
		arms.push_back(Arm());
		arms[armCount].setArmNumber(armCount);
	}
}

Motor::~Motor() {
	for (auto& arm : arms) {
		if(arm.accessArm().joinable())
			arm.accessArm().join();
	}
}

void Motor::setCommandList(CommandQueue* cL) {
	commandList = cL;
}

void Motor::start() {
	for (auto& arm : arms) {
		arm.accessArm() = thread(&Motor::execute, this, arm.getArmNumber());
	}
}

void Motor::execute(int arm_number) {
	{
		unique_lock <mutex> lock1(beforeStart);
		while (!commandList->accessReadyToSend())
			commandList->accessCV().wait(lock1);
	}
	
	while(!commandList->isEmpty(arm_number))
	{
		{
			unique_lock <mutex> lock2(motorActionLock);
			commandList->getFront(arm_number).printCommand();
			this_thread::sleep_for(chrono::milliseconds(commandList->getFront(arm_number).getTime()));
		}
		commandList->pop(arm_number);
	}
}

// ControlPanel Function Definitons
void ControlPanel::startProgram(string file_name) {
	cout << "Motor Command Program" << endl;
	cout << "---------------------" << endl;
	cout << endl;
	cout << "Progress" << endl;
	cout << "--------" << endl;
	main_motor.setCommandList(central_processor.getCommandList());
	main_motor.start();
	central_processor.readFile(file_name);
	central_processor.parseFile();
	cout << "Deciphering Commands" << endl;
	central_processor.decodeOpcode();
}