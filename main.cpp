#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>

#include <SFML/Graphics.hpp>

sf::Mutex spritesMutex;
sf::Mutex mousePressMutex;
sf::Mutex positionMutex;

struct Position
{
	std::array<std::array<std::string, 8>, 8> board;
	std::map<char, std::array<bool, 3>> castlingRights; //0 is king, 1 is left rook, 2 is right rook
	char turnPlayer;
	bool enPassantAllowed;
	std::array<int, 2> enPassantSquare;
	std::array<std::string, 8>& operator[](int x)
	{
		return board[x];
	}
	bool operator==(Position other)
	{
		return board == other.board
		&& castlingRights == other.castlingRights
		&& turnPlayer == other.turnPlayer
		&& enPassantAllowed == other.enPassantAllowed
		&& enPassantSquare == other.enPassantSquare;
	}
	void toggleTurn()
	{
		if (turnPlayer == 'l') turnPlayer = 'd';
		else if (turnPlayer == 'd') turnPlayer = 'l';
	}
};

struct MousePress
{
	std::array<int, 2> initialPosition;
	bool pressed;
};

struct RenderThreadParam
{
	sf::RenderWindow &root;
	Position &position;
	std::map<std::string, sf::Sprite> &sprites;
	MousePress &mousePress;
};

void renderingThread(RenderThreadParam param)
{
	sf::RenderWindow &root = param.root;
	Position &position = param.position;
	std::map<std::string, sf::Sprite> &sprites = param.sprites;
	MousePress &mousePress = param.mousePress;

	root.setActive(true);

	while (root.isOpen())
	{
		root.clear(sf::Color::Green);
		spritesMutex.lock();
		mousePressMutex.lock();
		root.draw(sprites["board"]);
	
		for (int x{0}; x<8; x++)
		{
			for (int y{0}; y<8; y++)
			{
				if (position[x][y] != "")
				{
					if (mousePress.pressed 
					&& mousePress.initialPosition[0] == x 
					&& mousePress.initialPosition[1] == y)
					{
						auto mousePosition = sf::Mouse::getPosition(root);
						sprites[position[x][y]].setPosition(mousePosition.x-22.5f, mousePosition.y-22.5f);
					}
					else

					{
						sprites[position[x][y]].setPosition(y*45.f, x*45.f);
					}
					root.draw(sprites[position[x][y]]);
				}
			}
		}
		spritesMutex.unlock();
		mousePressMutex.unlock();
		root.display();
	}
}

bool isCheck(Position &position);

void updateNewPosition(Position &newPosition, int x, int y, int i, int j)
{
	newPosition[x+i][y+j] = "";
	std::swap(newPosition[x+i][y+j], newPosition[x][y]);
	if (newPosition[0][4] != "kd") newPosition.castlingRights['d'][0] = false; 
	if (newPosition[0][0] != "rd") newPosition.castlingRights['d'][1] = false;
	if (newPosition[0][7] != "rd") newPosition.castlingRights['d'][2] = false;
	if (newPosition[7][4] != "kl") newPosition.castlingRights['l'][0] = false;  
	if (newPosition[7][0] != "rl") newPosition.castlingRights['l'][1] = false;
	if (newPosition[7][7] != "rl") newPosition.castlingRights['l'][2] = false; 
	newPosition.enPassantAllowed = false;
	newPosition.toggleTurn();
}

struct MoveGenerator
{
	Position position;
	bool ignoreCheck;
	int x;
	int y;
	int i;
	int pos;
	bool done;
	MoveGenerator(Position _position, bool _ignoreCheck = true)
	{
		position = _position;
		ignoreCheck = _ignoreCheck;
		x = 0;
		y = 0;
		i = 0;
		pos = 0;
		done = false;
	}
	Position next()
	{
		if (position[x][y] != "" && position[x][y][1] == position.turnPlayer)
		{
			Position newPosition = position;
			if (position[x][y][0] == 'r' || position[x][y][0] == 'q') //rook and half a queen
			{
				while (pos < 1)
				{
					pos = 0;
					i++;
					if (i >= (8-x))
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x+i][y] != "") {
							if (position[x+i][y][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, i, 0);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position;
							}
							pos++;
							i = 0;
						}
						else
						{
							updateNewPosition(newPosition, x, y, i, 0);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
				while (pos < 2)
				{
					pos = 1;
					i++;
					if (i > x)
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x-i][y] != "") {
							if (position[x-i][y][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, -i, 0);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0;
						}
						else
						{
							updateNewPosition(newPosition, x, y, -i, 0);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
				while (pos < 3)
				{
					pos = 2;
					i++;
					if (i >= (8-y))
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x][y+i] != "") {
							if (position[x][y+i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, 0, i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, 0, i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position;
						}
					}
				}
				while (pos < 4)
				{
					pos = 3;
					i++;
					if (i > y)
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x][y-i] != "") {
							if (position[x][y-i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, 0, -i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, 0, -i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
		
			}
			if (position[x][y][0] == 'b' || position[x][y][0] == 'q') //bishop and half a queen
			{
				while (pos < 5)
				{
					pos = 4;
					i++;
					if (i >= (8-x) || i>= (8-y))
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x+i][y+i] != "") {
							if (position[x+i][y+i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, i, i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, i, i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
				while (pos < 6)
				{
					pos = 5;
					i++;
					if (i > x || i > y)
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x-i][y-i] != "") {	
							if (position[x-i][y-i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, -i, -i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, -i, -i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
				while (pos < 7)
				{
					pos = 6;
					i++;
					if (i >= (8-x) || i > y)
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x+i][y-i] != "") {
							if (position[x+i][y-i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, i, -i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, i, -i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
				while (pos < 8)
				{
					pos = 7;
					i++;
					if (i > x || i >= (8-y))
					{
						pos++;
						i = 0;
					} 
					else
					{
						if (position[x-i][y+i] != "") {
							if (position[x-i][y+i][1] != position.turnPlayer)
							{
								updateNewPosition(newPosition, x, y, -i, i);
								if (ignoreCheck || !isCheck(newPosition)) {
									pos++;
									i = 0;
									return newPosition;
								}
								newPosition = position; 
							}
							pos++;
							i = 0; 
						}
						else
						{
							updateNewPosition(newPosition, x, y, -i, i);
							if (ignoreCheck || !isCheck(newPosition)) return newPosition;
							newPosition = position; 
						}
					}
				}
			}
			if (position[x][y][0] == 'n') //knight
			{
				std::array<std::array<int, 2>, 8> moves = {{{1, 2}, {2, 1}, {-1, 2}, {2, -1}, {1, -2}, {-2, 1}, {-1, -2}, {-2, -1}}};
				while (pos < 9)
				{
					pos = 8;
					if (i >= 8)
					{
						pos++;
						i = 0;
						break;
					} 
					else
					{
						while (i < 8 && (x+moves[i][0] < 0 || x+moves[i][0] > 7
						|| y+moves[i][1] < 0 || y+moves[i][1] > 7)) i++;
						if (i < 8) {
							if (position[x+moves[i][0]][y+moves[i][1]] != "") {
								if (position[x+moves[i][0]][y+moves[i][1]][1] != position.turnPlayer)
								{
									updateNewPosition(newPosition, x, y, moves[i][0], moves[i][1]);
									if (ignoreCheck || !isCheck(newPosition)) 
									{
										i++;
										return newPosition;
									}
									newPosition = position; 
								}
							}
							else
							{
								updateNewPosition(newPosition, x, y, moves[i][0], moves[i][1]);
								if (ignoreCheck || !isCheck(newPosition)) 
								{
									i++;
									return newPosition;
								}
								newPosition = position; 
							}
						}
						else
						{
							pos++;
							i = 0;
						}
					}
					i++;
				}
			}
			if (position[x][y][0] == 'k') //king
			{
				std::array<std::array<int, 2>, 8> moves = {{{1, 1}, {1, 0}, {1, -1}, {0, 1}, {0, -1}, {-1, 1}, {-1, 0}, {-1, -1}}};
				while (pos < 10)
				{
					pos = 9;
					if (i >= 8)
					{
						pos++;
						i = 0;
						break;
					} 
					else
					{
						while (i < 8 && (x+moves[i][0] < 0 || x+moves[i][0] > 7
						|| y+moves[i][1] < 0 || y+moves[i][1] > 7)) i++;
						if (i < 8) {
							if (position[x+moves[i][0]][y+moves[i][1]] != "") {
								if (position[x+moves[i][0]][y+moves[i][1]][1] != position.turnPlayer)
								{
									updateNewPosition(newPosition, x, y, moves[i][0], moves[i][1]);
									if (ignoreCheck || !isCheck(newPosition)) 
									{
										i++;
										return newPosition;
									}
									newPosition = position; 
								}
							}
							else
							{
								updateNewPosition(newPosition, x, y, moves[i][0], moves[i][1]);
								if (ignoreCheck || !isCheck(newPosition)) 
								{
									i++;
									return newPosition;
								}
								newPosition = position; 
							}
						}
						else
						{
							pos++;
							i = 0;
						}
					}
					i++;
				}
				while (pos < 11 && position.castlingRights[position.turnPlayer][0])
				{
					pos = 10;
					if (i > 1)
					{
						pos++;
						i = 0;
						break;
					}
					else
					{
						if (i == 0 && position.castlingRights[position.turnPlayer][2]
						&& position[x][y+1] == "" && position[x][y+2] == "")
						{
							updateNewPosition(newPosition, x, y, 0, 2);
							std::swap(newPosition[x][y+1], newPosition[x][y+3]);
							if (ignoreCheck || !isCheck(newPosition)) 
							{
								i++;
								return newPosition;
							}
							newPosition = position; 
						}
						if (i == 1 && position.castlingRights[position.turnPlayer][1] && position[x][y-1] == "" 
						&& position[x][y-2] == "" && position[x][y-3] == "")
						{
							updateNewPosition(newPosition, x, y, 0, -2);
							std::swap(newPosition[x][y-1], newPosition[x][y-4]);
							if (ignoreCheck || !isCheck(newPosition)) 
							{
								i++;
								return newPosition;
							}
							newPosition = position; 
						}	 
					}
					i++;
				}
			}
			if(position[x][y] == "pd") //dark pawn
			{
				while (pos < 12) 
				{
					pos = 11;
					if (i > 5)
					{
						pos++;
						i = 0;
						break;
					}
					if (i == 0 && x == 1 && position[x+1][y] == "" && position[x+2][y] == "")
					{
						updateNewPosition(newPosition, x, y, 2, 0); 
						newPosition.enPassantAllowed = true;
						newPosition.enPassantSquare = {2, y};
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 1 && x < 7 && position[x+1][y] == "")
					{
						if (x+1 == 7) newPosition[x][y] = "qd";
						updateNewPosition(newPosition, x, y, 1, 0);		
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 2 && x < 7 && y > 0 && position[x+1][y-1] != "" && position[x+1][y-1][1] == 'l')
					{
						if (x+1 == 7) newPosition[x][y] = "qd";
						updateNewPosition(newPosition, x, y, 1, -1);
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 3 && x < 7 && y < 7 && position[x+1][y+1] != "" && position[x+1][y+1][1] == 'l')
					{
						if (x+1 == 7) newPosition[x][y] = "qd"; 
						updateNewPosition(newPosition, x, y, 1, 1);
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
								newPosition = position; 
					}
					if (newPosition.enPassantAllowed && i == 4 
					&& newPosition.enPassantSquare[0] == x+1 
					&& newPosition.enPassantSquare[1] == y-1)
					{
						updateNewPosition(newPosition, x, y, 1, -1);
						newPosition[x][y-1] = "";
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (newPosition.enPassantAllowed && i == 5 
					&& newPosition.enPassantSquare[0] == x+1 
					&& newPosition.enPassantSquare[1] == y+1) 
					{
						updateNewPosition(newPosition, x, y, 1, 1);
						newPosition[x][y+1] = "";
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					i++;
				}
			}
			if(position[x][y] == "pl") //light pawn
			{	
				while (pos < 13)
				{
					pos = 12;
					if (i > 5)
					{
						pos++;
						i = 0;
						break;
					}
					if (i == 0 && x == 6 && position[x-1][y] == "" && position[x-2][y] == "")
					{
						updateNewPosition(newPosition, x, y, -2, 0);
						newPosition.enPassantAllowed = true;
						newPosition.enPassantSquare = {5, y};
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 1 && x > 0 && position[x-1][y] == "")
					{
						if (x-1 == 0) newPosition[x][y] = "ql";
						updateNewPosition(newPosition, x, y, -1, 0);
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 2 && x > 0 && y > 0 && position[x-1][y-1] != "" && position[x-1][y-1][1] == 'd')
					{
						if (x-1 == 0) newPosition[x][y] = "ql";
						updateNewPosition(newPosition, x, y, -1, -1);
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (i == 3 && x > 0 && y < 7 && position[x-1][y+1] != "" && position[x-1][y+1][1] == 'd')
					{
						if (x-1 == 0) newPosition[x][y] = "ql"; 
						updateNewPosition(newPosition, x, y, -1, 1);
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					if (newPosition.enPassantAllowed && i == 4 
					&& newPosition.enPassantSquare[0] == x-1 
					&& newPosition.enPassantSquare[1] == y-1)
					{
						updateNewPosition(newPosition, x, y, -1, -1);
						newPosition[x][y-1] = "";
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						} 
						newPosition = position; 
					}
					if (newPosition.enPassantAllowed && i == 5 
					&& newPosition.enPassantSquare[0] == x-1 
					&& newPosition.enPassantSquare[1] == y+1) 
					{
						updateNewPosition(newPosition, x, y, -1, 1);
						newPosition[x][y+1] = "";
						if (ignoreCheck || !isCheck(newPosition)) 
						{
							i++;
							return newPosition;
						}
						newPosition = position; 
					}
					i++;
				}
			} 
		}
		pos = 0;
		i = 0;
		x++;	
		if (x == 8)
		{
			x = 0;
			y++;
			if (y == 8) done = true;
		}
		if (!done) return next();
		return Position();
	}
};

bool isCheck(Position &position)
{
	MoveGenerator moveGenerator = {position, true};
	bool check = false;
	Position e = moveGenerator.next();
	while (!moveGenerator.done)
	{
		bool k = false;
		for (int x{0}; x<8; x++)
		{
			for (int y{0}; y<8; y++)
			{
				if (e[x][y] != ""
				&& e[x][y][0] == 'k'
				&& e[x][y][1] != position.turnPlayer) k = true;
			}
		}
		if (!k)
		{
			check = true;
			break;
		}
		e = moveGenerator.next();
	}
	return check;
}


float evaluate(Position &position, int depth, float alpha, float beta)
{
	if (depth > 0) {
		MoveGenerator moveGenerator = {position, true};
		Position move = moveGenerator.next();
		if (moveGenerator.done && !isCheck(move)) return 0.f;
		float value;
		if (position.turnPlayer == 'l')
		{
			value = -500.f;
			while (!moveGenerator.done)
			{
				value = std::max(value, evaluate(move, depth-1, alpha, beta));
				alpha = std::max(alpha, value);
				if (beta <= alpha) break;
				move = moveGenerator.next();
			}
		}
		if (position.turnPlayer == 'd')
		{
			value = 500.f;
			while (!moveGenerator.done)
			{
				value = std::min(value, evaluate(move, depth-1, alpha, beta));
				beta = std::min(beta, value);
				if (beta <= alpha) break;
				move = moveGenerator.next();
			}
		}
		return value;
	}
	std::map<char, float> valueMap = {{'k', 200.f},
									  {'q', 9.f},
									  {'r', 5.f},
									  {'b', 3.f},
									  {'n', 3.f},
									  {'p', 1.f},};
	float value = 0.f;
	for (int x{0}; x<8; x++)
	{
		for (int y{0}; y<8; y++)
		{
			if (position[x][y] == "")
			{
				continue;
			}
			float xQuality = 3.5f-fabs(static_cast<float>(x)-3.5f);
			float yQuality = 3.5f-fabs(static_cast<float>(y)-3.5f);
			if (position[x][y] == "pd") yQuality = static_cast<float>(y)/2.f;
			if (position[x][y] == "pl") yQuality = static_cast<float>(7-y)/2.f;
			float multiplier = (10.f+((xQuality+yQuality)/2.f))/10.f;
			if (position[x][y][0] == 'r') multiplier = 1.f;
			if (position[x][y][0] == 'k') multiplier = 1.f;
			if (position[x][y][1] == 'd') multiplier = multiplier * -1.f;
			value += valueMap[position[x][y][0]] * multiplier;
		}
	}
	return value;
}

Position generateBotMove(Position &position, int depth)
{
	MoveGenerator moveGenerator = {position, false};
	std::vector<Position> moves = {};
	Position move = moveGenerator.next();
	if (moveGenerator.done) //no legal moves
	{
		return position;
	}
	while (!moveGenerator.done)
	{
		moves.push_back(move);
		move = moveGenerator.next();
	}
	std::vector<float> values = {};
	for (Position &e: moves)
	{
		values.push_back(evaluate(e, depth-1, -500.f, 500.f));
	}
	auto best = values.begin();
	if (position.turnPlayer == 'l')
	{
		best = std::max_element(values.begin(), values.end()); //l player wants highest value
	}
	else
	{
		best = std::min_element(values.begin(), values.end()); //d player wants lowest value  
	}
	return moves[std::distance(values.begin(), best)];
}
			
int main()
{
	std::map<std::string, sf::Texture> textures = {{"kd", sf::Texture()},
											  	   {"kl", sf::Texture()},
 											 	   {"qd", sf::Texture()},
												   {"ql", sf::Texture()},
												   {"rd", sf::Texture()},
												   {"rl", sf::Texture()},
												   {"bd", sf::Texture()},
												   {"bl", sf::Texture()},
												   {"nd", sf::Texture()},
						  						   {"nl", sf::Texture()},
						 						   {"pd", sf::Texture()},
						   						   {"pl", sf::Texture()},
						   						   {"board", sf::Texture()},
	};

	for (auto& x: textures)
	{
		x.second.loadFromFile("sprites/"+x.first+".png");
	}

	std::array<std::array<std::string, 8>, 8> board = {"rd", "nd", "bd", "qd", "kd", "bd", "nd", "rd",
								   					   "pd", "pd", "pd", "pd", "pd", "pd", "pd", "pd",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
	   							   					   "pl", "pl", "pl", "pl", "pl", "pl", "pl", "pl",
							   						   "rl", "nl", "bl", "ql", "kl", "bl", "nl", "rl",
	};

	/*std::array<std::array<std::string, 8>, 8> board = {"", "", "", "", "", "", "rd", "rd",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "kd",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
														 "",   "",   "",   "",   "",   "",   "",   "",
							   						   "", "", "", "", "kl", "", "", "",
	};*/

	Position position = {board, {{'l', {true, true, true}}, {'d', {true, true, true}}}, 'l', false, {0, 0}};

	std::map<std::string, sf::Sprite> sprites = {};

	for (auto& x: textures)
	{
		sprites[x.first] = sf::Sprite(x.second);
	}

	MousePress mousePress = {{0,0}, false};

	std::cout << "Difficulty (type a number): " << std::endl;

	std::cout << "(1) 3 year old: " << std::endl;
	std::cout << "(2) Idiot: " << std::endl;
	std::cout << "(3) Easy: " << std::endl;
	std::cout << "(4) Hard: " << std::endl;
	std::cout << "(5) Challenging" << std::endl;
	std::cout << "Warning: challenging mode may take a long time between turns" << std::endl;


	std::string temp;
	std::getline(std::cin, temp);

	int difficulty = 2;

	for (char &e: temp)
		if ('0' < e && e <= '9')
			difficulty = e - '0';

	sf::RenderWindow root(sf::VideoMode(360, 360), "Chess");

	root.setActive(false);

	RenderThreadParam renderParam = {root, position, sprites, mousePress};
	sf::Thread renderThread(&renderingThread, renderParam);
	renderThread.launch();

	while (root.isOpen())
	{
		sf::Event event; 
		while (root.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				root.close();
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					mousePressMutex.lock();
					mousePress.pressed = true;
					auto mousePosition = sf::Mouse::getPosition(root);
					mousePress.initialPosition = {static_cast<int>(mousePosition.y/45.f),
												  static_cast<int>(mousePosition.x/45.f)};
					mousePressMutex.unlock();
				}
			}
			if (event.type == sf::Event::MouseButtonReleased)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{		
					auto mousePosition = sf::Mouse::getPosition(root);
					std::array<int, 2> finalPosition = {std::min(std::max(static_cast<int>(mousePosition.y/45.f), 0), 7),
												  		std::min(std::max(static_cast<int>(mousePosition.x/45.f), 0), 7)};
					Position newPosition = position; 
					newPosition.enPassantAllowed = false;
					if (newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]] != "")
			 		{
						if (newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]][0] == 'k')
						{
							if (mousePress.initialPosition[1] == 4 && finalPosition[1] == 6) //if kingside castle attempt
							{
								std::swap(newPosition[mousePress.initialPosition[0]][7], 
								newPosition[mousePress.initialPosition[0]][5]); //move rook
							}
							if (mousePress.initialPosition[1] == 4 && finalPosition[1] == 2) //if queenside castle attempt
							{
								std::swap(newPosition[mousePress.initialPosition[0]][0], 
								newPosition[mousePress.initialPosition[0]][3]); //move rook
							} 
						}
						if (newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]] == "pd")
						{
							if (finalPosition[0] == 7) newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]] = "qd";
							if (mousePress.initialPosition[0] == 1 && finalPosition[0] == 3 && mousePress.initialPosition[1] == finalPosition[1])
							{
								newPosition.enPassantAllowed = true;
								newPosition.enPassantSquare = {2, finalPosition[1]};
							}
							if (mousePress.initialPosition[1] != finalPosition[1] && newPosition[finalPosition[0]][finalPosition[1]]	== "")
							   newPosition[4][finalPosition[1]] = ""; 
						}  
						if (newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]] == "pl")
						{
							if (finalPosition[0] == 0) newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]] = "ql";
							if (mousePress.initialPosition[0] == 6 && finalPosition[0] == 4 && mousePress.initialPosition[1] == finalPosition[1])
							{
								newPosition.enPassantAllowed = true;
								newPosition.enPassantSquare = {5, finalPosition[1]}; 
							}
							if (mousePress.initialPosition[1] != finalPosition[1] && newPosition[finalPosition[0]][finalPosition[1]] == "")
							   newPosition[3][finalPosition[1]] = "";
						}  	
					}

					newPosition.toggleTurn(); 
					newPosition[finalPosition[0]][finalPosition[1]] = "";
					std::swap(newPosition[mousePress.initialPosition[0]][mousePress.initialPosition[1]],
					newPosition[finalPosition[0]][finalPosition[1]]);

					if (newPosition[0][4] != "kd") newPosition.castlingRights['d'][0] = false;  
					if (newPosition[0][0] != "rd") newPosition.castlingRights['d'][1] = false;
					if (newPosition[0][7] != "rd") newPosition.castlingRights['d'][2] = false;
					if (newPosition[7][4] != "kl") newPosition.castlingRights['l'][0] = false;  
					if (newPosition[7][0] != "rl") newPosition.castlingRights['l'][1] = false;
					if (newPosition[7][7] != "rl") newPosition.castlingRights['l'][2] = false; 

					MoveGenerator moveGenerator = {position, false}; 

					Position e = moveGenerator.next();
					while (!moveGenerator.done)
					{	
						if (e == newPosition)
						{
							positionMutex.lock();
							position = newPosition;
							auto start = std::chrono::high_resolution_clock::now();
							position = generateBotMove(position, difficulty);
							auto stop = std::chrono::high_resolution_clock::now(); 
							std::cout << std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() << std::endl;
							positionMutex.unlock();
							break;
						}
						e = moveGenerator.next();
					}
					mousePressMutex.lock();
					mousePress.pressed = false;
					mousePressMutex.unlock();	
				}
			}
		}
	}
	return 0;
}
