/**
 * @author	Steven Clark
 * @File	spamdetector.cpp
 * @brief	Detects which messages in a file of messages contain spam keywords.
 * Uses a Deterministic Finite Automaton state machine to identify message records
 * containing a pre-determined set of spam keywords
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <list>

using std::istream;
using std::cout;
using std::cerr;
using std::vector;
using std::endl;
using std::cin;
using std::string;
using std::stringstream;
using std::list;

//Functions used to identify input symbols (individually or grouped) for transitions
//First parameter is an input character to check, second an integer to compare it against
//Using the parameters is optional
typedef bool(* charComparator)(char, int);

//Command functions that effect overall program state
//These are used to queue spam message IDs for later printing
typedef void(* charConsumer)(char);

/// @brief Returns true for all unhandled characters in the alphabet
/// @return true, in all cases
bool everything(char, int){ return true; }

/// @brief Returns true for all digit characters
/// @param c An input character to check.
/// @return true if character is in '0' through '9'
bool digits(char c, int){ return c >= '0' and c <= '9'; }

/// @brief Matches an input character against a single character literal
/// @param c An input character to check.
/// @param against An integer value to compare c to.  Usually from a character literal
/// @return true if a and c have same integer value
bool justChar(char c, int against){ return c == against; }

/// @brief Is this character a printing whitespace character
/// @param c an input character to check
/// @return true if c is space, tab, carriage return, or newline
bool whitespace(char c, int){
	return c == ' ' or c == '\t' or c == '\r' or c == '\n';
}

/// @brief Is this character one of the delimiters for a spam keyword
/// @param c An input symbol to check.
/// @return true if c is space or double quote
bool delimiters(char c, int){ return c==' ' or c=='\"'; }


/// @brief Holds a finite automata state and all it's outgoing transitions.
class DFAstate{
protected:
	/// @brief Holds the definition of an outgoing transition function
	class transitionRecord{
	public:
		charComparator onSymbols;///< @brief Input symbol identification trigger function.
		DFAstate* to;			///< @brief Destination state
		charConsumer doing;		///< @brief Optional edge action.
		int comparedTo;			///< @brief optional value to compare input to (needed to identify single character with one function)

		/// @brief Builds a new transition function from it's defining parameters
		transitionRecord(charComparator which, DFAstate &where, charConsumer does = NULL, int what = -129){
			onSymbols = which; to = &where; doing = does; comparedTo = what;
		}
	};

	/// @brief The in order list of all the state's outgoing transitions
	vector<transitionRecord> transitions;
public:
	/// @brief A user-readable ID for this state.
	string name;

	/// @builds a name for this function by concatenating an integer onto a string, for naming sequences of states.
	/// @param newname String literal of the prefix of the name
	///	@param suffix An integer suffix for the name
	void iteratedname(const char* newname, int suffix){
		stringstream ss;
		ss << newname << suffix;
		name = ss.str();
	}

	/// @brief Adds a new outgoing transition to this state.
	/// @note Previously defined transitions will take precedence and catch whichever characters they identify, removing those from subsequent transitions
	/// @param which Pointer to a function that returns true if an input symbol triggers this transition
	/// @param where Reference to the detination state of this transition.
	/// @param does An optional pointer to an action function that is executed on this transition.  Null by default
	/// @param what An optional value to compare input characters against if the trigger function requires it.  Invalid for that use by default.
	void addTransition(charComparator which, DFAstate &where, charConsumer does = NULL, int what = -129){
		transitions.push_back(*(new transitionRecord(which, where, does, what)));
	}

	/// @brief Take the outgoing transition from this state given an input symbol.
	/// @param c An input symbol to transition with.
	/// @return A pointer to the next state in the automaton, NULL if unhandled
	/// @note While the structure of the automaton is nondeterministic in theory, this function interprets that structure in a strictly deterministic fashion.
	DFAstate *transitionWithChar(char c){
		if(transitions.empty())
			return NULL;
		for(vector<transitionRecord>::iterator i = transitions.begin(); i != transitions.end(); i++){
			if(i->onSymbols(c,i->comparedTo)){
				if(i->doing != NULL)
					i->doing(c);
				return i->to;
			}
		}
		return NULL;
	}
};


//Globals:
/// @brief holds a queue of spam message IDs as they are identified
list<int> spamMessages;

/// @brief The parsed message ID of the current message
int currentMessageNum;

/// @brief Transition action used to report a string has been accepted
void sayAccepted(char){
	cout << "Accepted" << endl;
}

/// @brief Transition action resetting the current message ID to zero
void newMsg(char){
	currentMessageNum = 0;
}

/// @brief Transition action making the given input symbol digit the new ones place of the current message ID
/// @param c the ASCII digit to reinterpret as the new one's place
/// @note previous value multiplied by 10 become the 10s place and beyond
void handleMIDdig(char c){
	int digit = c - '0';
	currentMessageNum *= 10;
	currentMessageNum += digit;
}

/// @brief Transition action adding the current message ID to the list of spam IDs
void recordSpam(char){
	spamMessages.push_back(currentMessageNum);
}

/// @brief Reads input messages from "./messagefile.txt" and records if they are spam or not
/// @return Unix exit code, 0 for success
int main(int, char**){
	std::ifstream file;
	file.open("messagefile.txt");

	// The states of the automaton
	DFAstate start;
	DFAstate openDoc[5];
	DFAstate openDocID[7];
	DFAstate msg[3];
	DFAstate msgdig[2];
	DFAstate closeDocID[8];
	DFAstate subject;
	DFAstate notdelimited;
	DFAstate delimited;
	DFAstate free_stuff[5];
	DFAstate free_access[6];
	DFAstate free_software[8];
	DFAstate free_vacation[8];
	DFAstate free_trials[6];
	DFAstate win[4];
	DFAstate winners[3];
	DFAstate winnings[4];
	DFAstate isSpam;
	DFAstate closeDoc[5];
	DFAstate closeDocSpam[5];

	//set start as the start state
	DFAstate *currentstate = &start;

	int input;

	//give all the states printable names
	start.name = "start";
	subject.name = "subject";
	delimited.name = "delimited";
	notdelimited.name = "not-delimited";
	isSpam.name = "isSpam";
	for(int i=0; i< 5; ++i) openDoc[i].iteratedname("openDoc_", i);
	for(int i=0; i< 7; ++i) openDocID[i].iteratedname("openDocID_", i);
	for(int i=0; i< 3; ++i) msg[i].iteratedname("msg_", i);
	for(int i=0; i< 2; ++i) msgdig[i].iteratedname("msgdig_", i);
	for(int i=0; i< 8; ++i) closeDocID[i].iteratedname("closeDocID_", i);
	for(int i=0; i< 5; ++i) free_stuff[i].iteratedname("free_stuff_", i);
	for(int i=0; i< 6; ++i) free_access[i].iteratedname("free_access_", i);
	for(int i=0; i< 8; ++i) free_software[i].iteratedname("free_software_", i);
	for(int i=0; i< 8; ++i) free_vacation[i].iteratedname("free_vacation_", i);
	for(int i=0; i< 6; ++i) free_trials[i].iteratedname("free_trials_", i);
	for(int i=0; i< 4; ++i) win[i].iteratedname("win_", i);
	for(int i=0; i< 3; ++i) winners[i].iteratedname("winners_", i);
	for(int i=0; i< 4; ++i) winnings[i].iteratedname("winnings_", i);
	for(int i=0; i< 5; ++i) closeDoc[i].iteratedname("closeDoc_", i);
	for(int i=0; i< 5; ++i) closeDocSpam[i].iteratedname("closeDocSpam_", i);

	//Begin defining the transition functions

	//for all states in the header, if invalid input encountered reset to start of header
	start.addTransition(&justChar,openDoc[0],NULL,'<');//transition forward only on the given character
	start.addTransition(&everything,start);				//otherwise return to start
	openDoc[0].addTransition(&justChar,openDoc[1],NULL,'D');
	openDoc[0].addTransition(&everything,start);
	openDoc[1].addTransition(&justChar,openDoc[2],NULL,'O');
	openDoc[1].addTransition(&everything,start);
	openDoc[2].addTransition(&justChar,openDoc[3],NULL,'C');
	openDoc[2].addTransition(&everything,start);
	openDoc[3].addTransition(&justChar,openDoc[4],&newMsg,'>');
	openDoc[3].addTransition(&everything,start);
	openDoc[4].addTransition(&whitespace,openDoc[4]);//allow whitespace between tags for extra robustness

	openDoc[4].addTransition(&justChar, openDocID[0],NULL,'<');
	openDoc[4].addTransition(&everything, start);
	openDocID[0].addTransition(&justChar,openDocID[1],NULL,'D');
	openDocID[0].addTransition(&everything,start);
	openDocID[1].addTransition(&justChar,openDocID[2],NULL,'O');
	openDocID[1].addTransition(&everything,start);
	openDocID[2].addTransition(&justChar,openDocID[3],NULL,'C');
	openDocID[2].addTransition(&everything,start);
	openDocID[3].addTransition(&justChar,openDocID[4],NULL,'I');
	openDocID[3].addTransition(&everything,start);
	openDocID[4].addTransition(&justChar,openDocID[5],NULL,'D');
	openDocID[4].addTransition(&everything,start);
	openDocID[5].addTransition(&justChar,openDocID[6],NULL,'>');
	openDocID[5].addTransition(&everything,start);
	openDocID[6].addTransition(&whitespace,openDocID[6]);

	openDocID[6].addTransition(&justChar,msg[0],NULL, 'm');
	openDocID[6].addTransition(&everything,start);
	msg[0].addTransition(&justChar,msg[1],NULL,'s');
	msg[0].addTransition(&everything,start);
	msg[1].addTransition(&justChar,msg[2],NULL,'g');
	msg[1].addTransition(&everything,start);

	msg[2].addTransition(&digits,msgdig[0],&handleMIDdig);//parse first digit of the message ID
	msg[2].addTransition(&everything,start);
	msgdig[0].addTransition(&justChar,closeDocID[0],NULL,'<');
	msgdig[0].addTransition(&whitespace,msgdig[1]);
	msgdig[0].addTransition(&digits,msgdig[1],&handleMIDdig);//parse additional digits of the message ID
	msgdig[0].addTransition(&everything,start);
	msgdig[1].addTransition(&justChar,closeDocID[0],NULL,'<');
	msgdig[1].addTransition(&whitespace,msgdig[1]);
	msgdig[1].addTransition(&everything,start);
	closeDocID[0].addTransition(&justChar,closeDocID[1],NULL,'/');
	closeDocID[0].addTransition(&everything,start);
	closeDocID[1].addTransition(&justChar,closeDocID[2],NULL,'D');
	closeDocID[1].addTransition(&everything,start);
	closeDocID[2].addTransition(&justChar,closeDocID[3],NULL,'O');
	closeDocID[2].addTransition(&everything,start);
	closeDocID[3].addTransition(&justChar,closeDocID[4],NULL,'C');
	closeDocID[3].addTransition(&everything,start);
	closeDocID[4].addTransition(&justChar,closeDocID[5],NULL,'I');
	closeDocID[4].addTransition(&everything,start);
	closeDocID[5].addTransition(&justChar,closeDocID[6],NULL,'D');
	closeDocID[5].addTransition(&everything,start);
	closeDocID[6].addTransition(&justChar,closeDocID[7],NULL,'>');
	closeDocID[6].addTransition(&everything,start);

	//all states return to subject until a line of only whitespace is encountered.
	closeDocID[7].addTransition(&justChar, subject, NULL, '\n');
	closeDocID[7].addTransition(&everything,closeDocID[7]);
	subject.addTransition(&justChar, delimited, NULL, '\n');
	subject.addTransition(&whitespace,subject);
	subject.addTransition(&everything,closeDocID[7]);

	//Spam keywords must begin with a delimiter (space or doublequote)
	notdelimited.addTransition(&justChar, closeDoc[0], NULL, '<'); //if we process the open angle we start checking for the closing DOC tag
	notdelimited.addTransition(&delimiters, delimited); //if we process a delimter we go to that state
	notdelimited.addTransition(&everything,notdelimited); //otherwise we process all other input and stay put

	delimited.addTransition(&justChar, free_stuff[0], NULL, 'f'); //if we encounter 'f' check spam keywords beginning with f
	delimited.addTransition(&justChar, win[0], NULL, 'w');		//if we encounter 'w' check spam keywords starting with w
	delimited.addTransition(&justChar, closeDoc[0], NULL, '<');	//if we encounter the angle bracket check for end of document tag
	delimited.addTransition(&delimiters, delimited);			//stay in delimited if another delimiter enctountered
	delimited.addTransition(&everything, notdelimited);			//for all other cases go to notdelimited state

	//Once a message is declared spam it stays spam until the end of document
	isSpam.addTransition(&justChar, closeDocSpam[0], NULL, '<');
	isSpam.addTransition(&everything, isSpam);

	//check non-spam message for end of document.
	closeDoc[0].addTransition(&justChar, closeDoc[1], NULL, '/');
	closeDoc[0].addTransition(&delimiters, delimited);
	closeDoc[0].addTransition(&everything, notdelimited);
	closeDoc[1].addTransition(&justChar, closeDoc[2], NULL, 'D');
	closeDoc[1].addTransition(&delimiters, delimited);
	closeDoc[1].addTransition(&everything, notdelimited);
	closeDoc[2].addTransition(&justChar, closeDoc[3], NULL, 'O');
	closeDoc[2].addTransition(&delimiters, delimited);
	closeDoc[2].addTransition(&everything, notdelimited);
	closeDoc[3].addTransition(&justChar, closeDoc[4], NULL, 'C');
	closeDoc[3].addTransition(&delimiters, delimited);
	closeDoc[3].addTransition(&everything, notdelimited);
	closeDoc[4].addTransition(&justChar, start, NULL, '>');//if </DOC> completed return to start state
	closeDoc[4].addTransition(&delimiters, delimited);
	closeDoc[4].addTransition(&everything, notdelimited);

	//check spam message for end of document
	closeDocSpam[0].addTransition(&justChar, closeDocSpam[1], NULL, '/');
	closeDocSpam[0].addTransition(&everything,isSpam);
	closeDocSpam[1].addTransition(&justChar, closeDocSpam[2], NULL, 'D');
	closeDocSpam[1].addTransition(&everything,isSpam);
	closeDocSpam[2].addTransition(&justChar, closeDocSpam[3], NULL, 'O');
	closeDocSpam[2].addTransition(&everything,isSpam);
	closeDocSpam[3].addTransition(&justChar, closeDocSpam[4], NULL, 'C');
	closeDocSpam[3].addTransition(&everything,isSpam);
	closeDocSpam[4].addTransition(&justChar, start, NULL, '>');//if </DOC> completed return to start state
	closeDocSpam[4].addTransition(&everything,isSpam);

	// Spam keyword "win" and keyowrds starting in "winn"
	win[0].addTransition(&justChar, win[1], NULL, 'i');
	win[0].addTransition(&delimiters, delimited);
	win[0].addTransition(&everything, notdelimited);
	win[1].addTransition(&justChar, win[2], NULL, 'n');
	win[1].addTransition(&delimiters, delimited);
	win[1].addTransition(&everything, notdelimited);
	win[2].addTransition(&justChar,win[3],NULL, 'n');
	win[2].addTransition(&delimiters, isSpam, &recordSpam);
	win[2].addTransition(&everything, notdelimited);
	win[3].addTransition(&justChar, winners[0], NULL, 'e');
	win[3].addTransition(&justChar, winnings[0], NULL, 'i');
	win[3].addTransition(&delimiters, delimited);
	win[3].addTransition(&everything, notdelimited);

	//complete "winn" to "winners"
	winners[0].addTransition(&justChar,winners[1],NULL, 'r');
	winners[0].addTransition(&delimiters, delimited);
	winners[0].addTransition(&everything, notdelimited);
	winners[1].addTransition(&justChar, winners[2], NULL, 's');
	winners[1].addTransition(&delimiters, isSpam, &recordSpam);
	winners[1].addTransition(&everything, notdelimited);
	winners[2].addTransition(&delimiters, isSpam, &recordSpam);//when transitioning to the isSpam state add the current message ID to the list
	winners[2].addTransition(&everything, notdelimited);

	//complete "winn" to "winnings"
	winnings[0].addTransition(&justChar,winnings[1],NULL,'n');
	winnings[0].addTransition(&delimiters, delimited);
	winnings[0].addTransition(&everything, notdelimited);
	winnings[1].addTransition(&justChar,winnings[2],NULL,'g');
	winnings[1].addTransition(&delimiters, delimited);
	winnings[1].addTransition(&everything, notdelimited);
	winnings[2].addTransition(&justChar,winnings[3],NULL,'s');
	winnings[2].addTransition(&delimiters, delimited);
	winnings[2].addTransition(&everything, notdelimited);
	winnings[3].addTransition(&delimiters, isSpam, &recordSpam);
	winnings[3].addTransition(&everything, notdelimited);

	//all keyphrases starting with "free "
	free_stuff[0].addTransition(&justChar, free_stuff[1], NULL, 'r');
	free_stuff[0].addTransition(&delimiters, delimited);
	free_stuff[0].addTransition(&everything, notdelimited);
	free_stuff[1].addTransition(&justChar, free_stuff[2], NULL, 'e');
	free_stuff[1].addTransition(&delimiters, delimited);
	free_stuff[1].addTransition(&everything, notdelimited);
	free_stuff[2].addTransition(&justChar, free_stuff[3], NULL, 'e');
	free_stuff[2].addTransition(&delimiters, delimited);
	free_stuff[2].addTransition(&everything, notdelimited);
	free_stuff[3].addTransition(&justChar, free_stuff[4], NULL, ' ');
	free_stuff[3].addTransition(&justChar, delimited, NULL, '\"');
	free_stuff[3].addTransition(&everything, notdelimited);
	free_stuff[4].addTransition(&justChar, free_stuff[0], NULL, 'f');
	free_stuff[4].addTransition(&justChar, win[0], NULL, 'w');
	free_stuff[4].addTransition(&justChar, closeDoc[0], NULL, '<');
	free_stuff[4].addTransition(&justChar, free_access[0], NULL, 'a');
	free_stuff[4].addTransition(&justChar, free_software[0], NULL, 's');
	free_stuff[4].addTransition(&justChar, free_trials[0], NULL, 't');
	free_stuff[4].addTransition(&justChar, free_vacation[0], NULL, 'v');
	free_stuff[4].addTransition(&delimiters, delimited);
	free_stuff[4].addTransition(&everything, notdelimited);

	free_access[0].addTransition(&justChar, free_access[1], NULL, 'c');
	free_access[0].addTransition(&delimiters,delimited);
	free_access[0].addTransition(&everything, notdelimited);
	free_access[1].addTransition(&justChar, free_access[2], NULL, 'c');
	free_access[1].addTransition(&delimiters,delimited);
	free_access[1].addTransition(&everything, notdelimited);
	free_access[2].addTransition(&justChar, free_access[3], NULL, 'e');
	free_access[2].addTransition(&delimiters,delimited);
	free_access[2].addTransition(&everything, notdelimited);
	free_access[3].addTransition(&justChar, free_access[4], NULL, 's');
	free_access[3].addTransition(&delimiters,delimited);
	free_access[3].addTransition(&everything, notdelimited);
	free_access[4].addTransition(&justChar, free_access[5], NULL, 's');
	free_access[4].addTransition(&delimiters,delimited);
	free_access[4].addTransition(&everything, notdelimited);
	free_access[5].addTransition(&delimiters, isSpam, &recordSpam);
	free_access[5].addTransition(&everything, notdelimited);

	free_software[0].addTransition(&justChar, free_software[1], NULL, 'o');
	free_software[1].addTransition(&justChar, free_software[2], NULL, 'f');
	free_software[2].addTransition(&justChar, free_software[3], NULL, 't');
	free_software[3].addTransition(&justChar, free_software[4], NULL, 'w');
	free_software[4].addTransition(&justChar, free_software[5], NULL, 'a');
	free_software[5].addTransition(&justChar, free_software[6], NULL, 'r');
	free_software[6].addTransition(&justChar, free_software[7], NULL, 'e');
	for(int i=0; i<7; ++i) free_software[i].addTransition(&delimiters, delimited);
	free_software[7].addTransition(&delimiters, isSpam, &recordSpam);
	for(int i=0; i<=7; ++i) free_software[i].addTransition(&everything, notdelimited);

	free_trials[0].addTransition(&justChar, free_trials[1], NULL, 'r');
	free_trials[1].addTransition(&justChar, free_trials[2], NULL, 'i');
	free_trials[2].addTransition(&justChar, free_trials[3], NULL, 'a');
	free_trials[3].addTransition(&justChar, free_trials[4], NULL, 'l');
	free_trials[4].addTransition(&justChar, free_trials[5], NULL, 's');
	for(int i=0; i< 5; ++i) free_trials[i].addTransition(&delimiters, delimited);
	free_trials[5].addTransition(&delimiters, isSpam, &recordSpam);
	for(int i=0; i<=5; ++i) free_trials[i].addTransition(&everything, notdelimited);

	free_vacation[0].addTransition(&justChar, free_vacation[1], NULL, 'a');
	free_vacation[1].addTransition(&justChar, free_vacation[2], NULL, 'c');
	free_vacation[2].addTransition(&justChar, free_vacation[3], NULL, 'a');
	free_vacation[3].addTransition(&justChar, free_vacation[4], NULL, 't');
	free_vacation[4].addTransition(&justChar, free_vacation[5], NULL, 'i');
	free_vacation[5].addTransition(&justChar, free_vacation[6], NULL, 'o');
	free_vacation[6].addTransition(&justChar, free_vacation[7], NULL, 'n');
	for(int i=0; i<7; ++i) free_vacation[i].addTransition(&delimiters, delimited);
	free_vacation[7].addTransition(&delimiters, isSpam, &recordSpam);
	for(int i=0; i<=7; ++i) free_vacation[i].addTransition(&everything, notdelimited);
	//finish defining the transition functions

	input = 0;

	//for each input character
	do{
		//If there was no transition function from the symbol (current state invalid)
		if(NULL == currentstate){
			//exit with error
			cerr << "Error: Unhandled symbol:" << (char)input << endl;
			return -1;
		}

		input = file.get();

		if(input >= 0){
			//print the "name" of the current state
			cout << '\"'<< currentstate->name << '\"';

			//print an arrow showing the input character for the transition
			cout << '-' << char(input) << "->";

			//transition out of the current state using the input symbol
			currentstate= currentstate->transitionWithChar(input);

		}else{
			//output <end> when an error was taken trying to get input from file.
			cout << "<end>" << endl;
		}

	}while( input >= 0);

	//report the spam message IDs
	cout << "The following messages were spam:";
	while(!(spamMessages.empty())){
		cout << ' '<< spamMessages.front();
		spamMessages.pop_front();
	}
	cout << endl;

	//exit successfully
	return 0;
}
