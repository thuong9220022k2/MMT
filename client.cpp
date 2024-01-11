#include <iostream>
#include <SFML/Graphics.hpp>
using namespace std;
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <jsoncpp/json/json.h>
#include <unistd.h>
#include <limits>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "type.h"
#include <math.h>
#include <time.h>
#include <stack>
#include <algorithm>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <thread>
#include <chrono>
int is_login = 0;
string USERNAME;
int USER_ID;
int USER_ID_COMPETIOR;
int SCORE;
int ROOM_ID;
std::mutex mtx;
std::condition_variable cv;
bool messageSent = false;
bool awaitingResponse = false;

int size = 56;
sf::Vector2f offset(28, 28);
sf::RenderWindow window;
int board[8][8] = {
    -1, -2, -3, -4, -5, -3, -2, -1,
    -6, -6, -6, -6, -6, -6, -6, -6,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    6, 6, 6, 6, 6, 6, 6, 6,
    1, 2, 3, 4, 5, 3, 2, 1};

sf::Texture t1, t2, t3;
sf::Sprite sBoard, sPositive;
typedef struct QuanCo
{
    sf::Sprite s;
    int index, cost;
};

QuanCo f[33];                  // mang luu cac quan co
sf::Vector2f positiveMove[32]; // vi tri cac nuoc co the di chuyen
int positiveCount;             // so nuoc co the di chuyen
std::stack<sf::Vector2f> posS; // luu tru vi tri cac nuoc di
std::stack<int> nS;            // luu tru index cua quan di
int scores1, scores2;
int count1, count2;
bool LuotChoi;
sf::Vector2f oldPos, newPos;
sf::Vector2f oldPostemp, newPostemp;
int n, click, count, check;
sf::Vector2i pos;
bool checktime;

int indexArray;
void move(int n, sf::Vector2f oldPos, sf::Vector2f newPos)
{
    posS.push(oldPos);
    posS.push(newPos);
    nS.push(n);
    int y = int((newPos - offset).y / size); // kiem tra xem co phong hau hay khong
    // phong hau cho tot
    if (y == 0 && f[n].index == 6)
    {
        nS.push(100); // de ty undo xoa phong hau di
        f[n].index = 4;
        f[n].cost = 90;
        f[n].s.setTextureRect(sf::IntRect(3 * size, size, size, size));
    }
    if (y == 7 && f[n].index == -6)
    {
        nS.push(-100);
        f[n].index = -4;
        f[n].cost = -90;
        f[n].s.setTextureRect(sf::IntRect(3 * size, 0, size, size));
    }
    for (int i = 0; i < 32; i++)
    {
        if (f[i].s.getPosition() == newPos)
        {
            f[i].s.setPosition(-100, -100); // di chuyen em bi an ra khoi man hinh
            posS.push(newPos);
            posS.push(sf::Vector2f(-100, -100));
            nS.push(i);
            break;
        }
    }
    f[n].s.setPosition(newPos);
}

void Undo()
{
    int n = nS.top();
    nS.pop();
    sf::Vector2f p = posS.top(); // kiem tra xem co = (-100,-100) => day la con bi an
    posS.pop();
    sf::Vector2f q = posS.top();
    posS.pop();
    if (n == 100)
    {
        n = nS.top();
        nS.pop();
        f[n].index = 6;
        f[n].cost = 10;
        f[n].s.setTextureRect(sf::IntRect(5 * size, size, size, size));
    }
    if (n == -100)
    {
        n = nS.top();
        nS.pop();
        f[n].index = -6;
        f[n].cost = -10;
        f[n].s.setTextureRect(sf::IntRect(5 * size, 0, size, size));
    }
    f[n].s.setPosition(q);

    if (p == sf::Vector2f(-100, -100))
        Undo(); // luc nay moi dy chuyen con an
}

void Create()
{
    positiveCount = 0; // so nuoc co the di ban dau duong nhien =0(chua chon gi ca)
    int k = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int n = board[i][j];
            if (!n)
                continue;
            int x = abs(n) - 1;
            int y = n > 0 ? 1 : 0;
            f[k].index = n;
            f[k].s.setTextureRect(sf::IntRect(size * x, size * y, size, size));
            f[k].s.setPosition(size * j + offset.x, size * i + offset.y);
            // cost
            int v = 0, g;
            g = abs(f[k].index);
            if (g == 1)
                v = 50;
            else if (g == 2 || g == 3)
                v = 30;
            else if (g == 4)
                v = 90;
            else if (g == 5)
                v = 900;
            else if (g == 6)
                v = 10;
            f[k].cost = f[k].index / g * v;
            k++;
        }
    }
}

void IncreasePositive(int i, int j)
{
    positiveMove[positiveCount] = sf::Vector2f(i * size, j * size) + offset;
    positiveCount++;
}
void PositiveXe(int n, int x, int y, int grid[9][9])
{
    for (int i = x + 1; i < 8; i++)
    {
        if (grid[i][y] != 0)
        {
            if (grid[i][y] * grid[x][y] < 0)
                IncreasePositive(i, y);
            break;
        }
        IncreasePositive(i, y);
    }
    for (int i = x - 1; i >= 0; i--)
    {
        if (grid[i][y] != 0)
        {
            if (grid[i][y] * grid[x][y] < 0)
                IncreasePositive(i, y);
            break;
        }
        IncreasePositive(i, y);
    }
    for (int j = y + 1; j < 8; j++)
    {
        if (grid[x][j] != 0)
        {
            if (grid[x][j] * grid[x][y] < 0)
                IncreasePositive(x, j);
            break;
        }
        IncreasePositive(x, j);
    }
    for (int j = y - 1; j >= 0; j--)
    {
        if (grid[x][j] != 0)
        {
            if (grid[x][j] * grid[x][y] < 0)
                IncreasePositive(x, j);
            break;
        }
        IncreasePositive(x, j);
    }
}
void PositiveTuong(int n, int x, int y, int grid[9][9])
{
    for (int i = x + 1, j = y + 1; (i < 8 && j < 8); i++, j++)
    {
        if (grid[i][j] != 0)
        {
            if (grid[i][j] * grid[x][y] < 0)
                IncreasePositive(i, j);
            break;
        }
        IncreasePositive(i, j);
    }
    for (int i = x + 1, j = y - 1; (i < 8 && j >= 0); i++, j--)
    {
        if (grid[i][j] != 0)
        {
            if (grid[i][j] * grid[x][y] < 0)
                IncreasePositive(i, j);
            break;
        }
        IncreasePositive(i, j);
    }
    for (int i = x - 1, j = y + 1; (i >= 0 && j < 8); i--, j++)
    {
        if (grid[i][j] != 0)
        {
            if (grid[i][j] * grid[x][y] < 0)
                IncreasePositive(i, j);
            break;
        }
        IncreasePositive(i, j);
    }
    for (int i = x - 1, j = y - 1; (i >= 0 && j >= 0); i--, j--)
    {
        if (grid[i][j] != 0)
        {
            if (grid[i][j] * grid[x][y] < 0)
                IncreasePositive(i, j);
            break;
        }
        IncreasePositive(i, j);
    }
}
void PositiveMa(int n, int x, int y, int grid[9][9])
{
    if ((grid[x + 2][y + 1] == 0 || grid[x][y] * grid[x + 2][y + 1] < 0) && x + 2 < 8 && y + 1 < 8)
        IncreasePositive(x + 2, y + 1);
    if ((grid[x + 2][y - 1] == 0 || grid[x][y] * grid[x + 2][y - 1] < 0) && y - 1 >= 0 && x + 2 < 8)
        IncreasePositive(x + 2, y - 1);
    if ((grid[x - 2][y + 1] == 0 || grid[x][y] * grid[x - 2][y + 1] < 0) && x - 2 >= 0 && y + 1 < 8)
        IncreasePositive(x - 2, y + 1);
    if ((grid[x - 2][y - 1] == 0 || grid[x][y] * grid[x - 2][y - 1] < 0) && x - 2 >= 0 && y - 1 >= 0)
        IncreasePositive(x - 2, y - 1);
    if ((grid[x + 1][y + 2] == 0 || grid[x][y] * grid[x + 1][y + 2] < 0) && x + 1 < 8 && y + 2 < 8)
        IncreasePositive(x + 1, y + 2);
    if ((grid[x - 1][y + 2] == 0 || grid[x][y] * grid[x - 1][y + 2] < 0) && x - 1 >= 0 && y + 2 < 8)
        IncreasePositive(x - 1, y + 2);
    if ((grid[x + 1][y - 2] == 0 || grid[x][y] * grid[x + 1][y - 2] < 0) && y - 2 >= 0 && x + 1 < 8)
        IncreasePositive(x + 1, y - 2);
    if ((grid[x - 1][y - 2] == 0 || grid[x][y] * grid[x - 1][y - 2] < 0) && x - 1 >= 0 && y - 2 >= 0)
        IncreasePositive(x - 1, y - 2);
}
void PositiveVua(int n, int x, int y, int grid[9][9])
{
    if ((grid[x + 1][y] == 0 || grid[x][y] * grid[x + 1][y] < 0) && x + 1 < 8)
        IncreasePositive(x + 1, y);
    if ((grid[x - 1][y] == 0 || grid[x][y] * grid[x - 1][y] < 0) && x - 1 >= 0)
        IncreasePositive(x - 1, y);
    if ((grid[x + 1][y + 1] == 0 || grid[x][y] * grid[x + 1][y + 1] < 0) && x + 1 < 8 && y + 1 < 8)
        IncreasePositive(x + 1, y + 1);
    if ((grid[x - 1][y + 1] == 0 || grid[x][y] * grid[x - 1][y + 1] < 0) && x - 1 >= 0 && y + 1 < 8)
        IncreasePositive(x - 1, y + 1);
    if ((grid[x][y + 1] == 0 || grid[x][y] * grid[x][y + 1] < 0) && y + 1 < 8)
        IncreasePositive(x, y + 1);
    if ((grid[x - 1][y - 1] == 0 || grid[x][y] * grid[x - 1][y - 1] < 0) && x - 1 >= 0 && y - 1 >= 0)
        IncreasePositive(x - 1, y - 1);
    if ((grid[x + 1][y - 1] == 0 || grid[x][y] * grid[x + 1][y - 1] < 0) && y - 1 >= 0 && x + 1 < 8)
        IncreasePositive(x + 1, y - 1);
    if ((grid[x][y - 1] == 0 || grid[x][y] * grid[x][y - 1] < 0) && y - 1 >= 0)
        IncreasePositive(x, y - 1);
}
void PositiveTot(int n, int x, int y, int grid[9][9])
{
    int k = grid[x][y] / abs(grid[x][y]); // 1 hoac -1
    if ((y == 1 || y == 6) && grid[x][y - k] == 0 && grid[x][y - 2 * k] == 0 && y - 2 * k >= 0 && y - 2 * k < 8)
        IncreasePositive(x, y - 2 * k);
    if (grid[x][y - k] == 0 && y - k >= 0 && y - k < 8)
        IncreasePositive(x, y - k);
    if (grid[x + 1][y - k] * grid[x][y] < 0 && y - k >= 0 && y - k < 8 && x + 1 < 8)
        IncreasePositive(x + 1, y - k);
    if (grid[x - 1][y - k] * grid[x][y] < 0 && y - k >= 0 && y - k < 8 && x - 1 >= 0)
        IncreasePositive(x - 1, y - k);
}

void PositiveMoving(int n)
{
    sf::Vector2f pos = f[n].s.getPosition() - offset;
    int x = pos.x / size;
    int y = pos.y / size;

    int grid[9][9]; // mang luoi(8x8) luu lai cac vi tri ban co:
    sf::Vector2i vtri;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            grid[i][j] = 0; // neu khong co quan co nao o O nay thi =0
    for (int j = 0; j < 32; j++)
    {
        vtri = sf::Vector2i(f[j].s.getPosition() - offset);
        grid[vtri.x / size][vtri.y / size] = f[j].index; // neu co = index cua quan co
    }

    if (abs(f[n].index) == 1)
        PositiveXe(n, x, y, grid); // xe
    else if (abs(f[n].index) == 2)
        PositiveMa(n, x, y, grid); // ma
    else if (abs(f[n].index) == 3)
        PositiveTuong(n, x, y, grid); // tuong
    else if (abs(f[n].index) == 4)    // hau: hop lai cac nuoc cua ca xe va tuong
    {
        PositiveXe(n, x, y, grid);
        PositiveTuong(n, x, y, grid);
    }
    else if (abs(f[n].index) == 5)
        PositiveVua(n, x, y, grid); // vua
    else
        PositiveTot(n, x, y, grid); // tot
}
// AI
int CostMove()
{
    int s = 0;
    for (int i = 0; i < 32; i++)
    {
        if (f[i].s.getPosition() == sf::Vector2f(-100, -100))
            continue; // neu no da bi out-> ko tinh diem
        s += f[i].cost;
    }
    return s;
}

int CalculateScore(sf::Vector2f newPos)
{
    for (int i = 0; i < 32; i++)
    {
        if (f[i].s.getPosition() == newPos)
        {
            if (f[i].index != 0)
            {
                f[i].cost > 0 ? scores1 += f[i].cost : scores2 += abs(f[i].cost);
                std::cout << "scores1: " << scores1 << " scores2: " << scores2 << std::endl;
            }
            return i;
        }
    }
    return -1;
}

int CheckWin()
{
    if (scores1 >= 1030)
        return 1;
    if (scores2 >= 1030)
        return 2;
    return 0;
}
int Alpha_Beta(int depth, bool luot, int alpha, int beta) // cat tia alpha beta
{
    if (depth == 0)
    {
        return CostMove();
    }
    sf::Vector2f positiveMovetemp[32]; // luu lai vi tri cac nuoc co the di
    if (luot == true)
    {
        int bestMove = -10000;        // gia cua bestMove ban dau
        for (int j = 16; j < 32; j++) // cac quan cua nguoi choi
        {
            if (f[j].s.getPosition() == sf::Vector2f(-100, -100))
                continue;
            PositiveMoving(j);
            int coun = positiveCount; // ta khong the dung PositiveCount vi no thay doi lien tuc khi ta de quy
            positiveCount = 0;
            for (int i = 0; i < coun; i++)
                positiveMovetemp[i] = positiveMove[i];
            for (int i = 0; i < coun; i++)
            {
                move(j, f[j].s.getPosition(), positiveMovetemp[i]);
                bestMove = std::max(bestMove, Alpha_Beta(depth - 1, !luot, alpha, beta));
                // undo
                Undo();
                alpha = std::max(alpha, bestMove);
                if (beta <= alpha)
                    return bestMove;
            }
        }
        return bestMove;
    }
    else
    {
        int bestMove = 10000;        // gia cua bestMove ban dau
        for (int j = 0; j < 16; j++) // quan cua may
        {
            if (f[j].s.getPosition() == sf::Vector2f(-100, -100))
                continue;
            PositiveMoving(j);
            int coun = positiveCount; // ta khong the dung PositiveCount vi no thay doi lien tuc khi ta de quy
            positiveCount = 0;
            for (int i = 0; i < coun; i++)
                positiveMovetemp[i] = positiveMove[i];
            for (int i = 0; i < coun; i++)
            {
                move(j, f[j].s.getPosition(), positiveMovetemp[i]);
                bestMove = std::min(bestMove, Alpha_Beta(depth - 1, !luot, alpha, beta));
                // undo
                Undo();
                beta = std::min(beta, bestMove);
                if (beta <= alpha)
                    return bestMove;
            }
        }
        return bestMove;
    }
}

sf::Vector2f getNextMove(bool luot) // tra ve nuoc di tot nhat theo chien luoc phia tren
{
    sf::Vector2f oldPos, newPos, oldPostemp, newPostemp; // ta can tim vi tri co minimax nho nhat de ung voi may( quan den)
    int minimaxtemp = 10000, minimax = 10000;
    int count1, n;
    sf::Vector2f positiveMovetemp[32];

    for (int i = 0; i < 16; i++)
    {
        if (f[i].s.getPosition() == sf::Vector2f(-100, -100))
            continue;
        //////
        PositiveMoving(i);
        count1 = positiveCount;
        positiveCount = 0;
        /// set///
        for (int k = 0; k < count1; k++)
            positiveMovetemp[k] = positiveMove[k];
        // set oldPos va newPos  tam thoi
        oldPostemp = f[i].s.getPosition();
        // newPostemp=positiveMove[0];
        for (int j = 0; j < count1; j++)
        {
            move(i, oldPostemp, positiveMovetemp[j]);
            int alpha = -9999, beta = 9999;
            int temp = Alpha_Beta(3, !luot, alpha, beta);
            if (minimaxtemp > temp)
            {
                newPostemp = positiveMovetemp[j];
                minimaxtemp = temp;
            }
            Undo();
        }
        if (minimax > minimaxtemp)
        {
            minimax = minimaxtemp;
            oldPos = oldPostemp;
            newPos = newPostemp;
            n = i;
        }
    }
    posS.push(oldPos);
    nS.push(n);
    return newPos;
}

void PlaySingle()
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "The Chess! Alpha Beta Pruning");
    t1.loadFromFile("figures3.png");
    t2.loadFromFile("board4.png");
    t3.loadFromFile("no.png");

    for (int i = 0; i < 32; i++)
    {
        // Tính toán vị trí cắt cho mỗi quân cờ trên texture
        f[i].s.setTexture(t1);
    }
    sBoard.setTexture(t2);
    sPositive.setTexture(t3);

    Create(); // khoi tao

    bool LuotChoi = true;
    int n = 0, click = 0, count = 0;
    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();
            ////move back//////
            if (e.type == sf::Event::KeyPressed)
                if (e.key.code == sf::Keyboard::BackSpace)
                {
                    Undo();
                }
            /////click///////
            if (e.type == sf::Event::MouseButtonPressed)
                if (e.key.code == sf::Mouse::Left)
                {
                    pos = sf::Mouse::getPosition(window) - sf::Vector2i(offset);
                    click++;
                }
        }
        if (LuotChoi == true)
        {
            if (click == 1)
            {
                bool isMove = false;
                for (int i = 16; i < 32; i++)
                {
                    if (f[i].s.getGlobalBounds().contains(pos.x + offset.x, pos.y + offset.y))
                    {
                        isMove = true;
                        n = i;
                        f[n].s.setColor(sf::Color::Green);
                        oldPos = f[n].s.getPosition();
                    }
                }
                if (!isMove)
                    click = 0;
                else
                {
                    PositiveMoving(n);
                    count = positiveCount;
                    positiveCount = 0;
                }
            }
            if (click == 2)
            {
                f[n].s.setColor(sf::Color::White);
                int x = pos.x / size;
                int y = pos.y / size;
                newPos = sf::Vector2f(x * size, y * size) + offset;
                // chi di chuyen trong vung positiveMove
                for (int i = 0; i < count; i++)
                {
                    if (positiveMove[i] == newPos)
                    {
                        CalculateScore(newPos);
                        if (CheckWin() != 0)
                        {
                            std::cout << "Game Over" << CheckWin() << std::endl;
                            return;
                        }
                        move(n, oldPos, newPos);
                        LuotChoi = !LuotChoi;
                    }
                }
                // reset
                count = 0;
                click = 0;
            }
        }

        else
        {
            newPos = getNextMove(LuotChoi);
            CalculateScore(newPos);
            if (CheckWin() != 0)
            {
                std::cout << "Game Over" << CheckWin() << std::endl;
                return;
            }
            int c = nS.top();
            nS.pop(); // lay dk thong tin roi xoa di
            oldPos = posS.top();
            posS.pop(); // vi ham move tu nhet trong stack roi
            move(c, oldPos, newPos);
            LuotChoi = !LuotChoi;
            // reset
            click = 0;
        }
        ////// draw  ///////
        window.draw(sBoard);
        for (int i = 0; i < count; i++)
        {
            sPositive.setPosition(positiveMove[i]);
            window.draw(sPositive);
        }
        for (int i = 0; i < 32; i++)
        {
            window.draw(f[i].s);
        }
        window.display();
    }
}

sf::Vector2f convertMove(sf::Vector2f pos)
{
    // Giả sử size là kích thước của một ô cờ (ví dụ: 56 pixels)
    int x = pos.x / size; // Chia để lấy chỉ số cột
    int y = pos.y / size; // Chia để lấy chỉ số hàng

    // Phản chiếu vị trí qua trung tâm của bàn cờ
    // Đối với bàn cờ 8x8, chỉ số mới sẽ là 7 - chỉ số hiện tại
    int newX = 7 - x;
    int newY = 7 - y;

    // Nhân với size và thêm offset để lấy vị trí pixel mới
    return sf::Vector2f(newX * size, newY * size) + offset;
}

void sendMove(int client_socket, int n, sf::Vector2f oldPos, sf::Vector2f newPos)
{
    Json::Value root;
    root["type"] = MOVE;
    root["user_id"] = USER_ID;
    root["room_id"] = ROOM_ID;
    root["n"] = n;
    root["oldPos_x"] = oldPos.x;
    root["oldPos_y"] = oldPos.y;
    root["newPos_x"] = newPos.x;
    root["newPos_y"] = newPos.y;
    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    std::cout << json_string << std::endl;
    send(client_socket, json_string.c_str(), json_string.length(), 0);
    awaitingResponse = true;
    messageSent = false;
    cout << "send" << endl;
}
void PlayBlack(int client_socket)
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "The Chess! Alpha Beta Pruning");
    t1.loadFromFile("figures5.png");
    t2.loadFromFile("board4.png");
    t3.loadFromFile("no.png");

    for (int i = 0; i < 32; i++)
    {
        // Tính toán vị trí cắt cho mỗi quân cờ trên texture
        f[i].s.setTexture(t1);
    }
    sBoard.setTexture(t2);
    sPositive.setTexture(t3);

    Create(); // khoi tao

    bool LuotChoi = true;
    int n = 0, click = 0, count = 0;
    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();
            ////move back//////
            if (e.type == sf::Event::KeyPressed)
                if (e.key.code == sf::Keyboard::BackSpace)
                {
                    Undo();
                }
            /////click///////
            if (e.type == sf::Event::MouseButtonPressed)
                if (e.key.code == sf::Mouse::Left)
                {
                    pos = sf::Mouse::getPosition(window) - sf::Vector2i(offset);
                    click++;
                }
        }
        if (LuotChoi == true)
        {
            if (click == 1)
            {
                bool isMove = false;
                for (int i = 16; i < 32; i++)
                {
                    if (f[i].s.getGlobalBounds().contains(pos.x + offset.x, pos.y + offset.y))
                    {
                        isMove = true;
                        n = i;
                        f[n].s.setColor(sf::Color::Green);
                        oldPos = f[n].s.getPosition();
                    }
                }
                if (!isMove)
                    click = 0;
                else
                {
                    PositiveMoving(n);
                    count = positiveCount;
                    positiveCount = 0;
                }
            }
            if (click == 2)
            {
                f[n].s.setColor(sf::Color::White);
                int x = pos.x / size;
                int y = pos.y / size;
                newPos = sf::Vector2f(x * size, y * size) + offset;
                // chi di chuyen trong vung positiveMove
                for (int i = 0; i < count; i++)
                {
                    if (positiveMove[i] == newPos)
                    {
                        CalculateScore(newPos);
                        if (CheckWin() != 0)
                        {
                            std::cout << "Game Over" << CheckWin() << std::endl;
                            return;
                        }
                        move(n, oldPos, newPos);
                        sendMove(client_socket, n, oldPos, newPos);
                        LuotChoi = !LuotChoi;
                    }
                }
                // reset
                count = 0;
                click = 0;
            }
        }

        else
        {
            auto start = std::chrono::high_resolution_clock::now();
            while (oldPostemp == sf::Vector2f())
            { // Kiểm tra nếu oldPostemp là null
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                if (elapsed.count() >= 30)
                { // Đã chờ đủ 30 giây
                    std::cout << "Timeout waiting for oldPostemp to have a value." << std::endl;
                    return; // Thoát hàm nếu quá thời gian chờ
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            sf::Vector2f oldPostemp1 = convertMove(oldPostemp);
            sf::Vector2f newPostemp1 = convertMove(newPostemp);
            // CalculateScore(newPostemp);
            // if (CheckWin() != 0)
            // {
            //     std::cout << "Game Over" << CheckWin() << std::endl;
            //     return;
            // }
            int c = nS.top();
            nS.pop(); // lay dk thong tin roi xoa di
            oldPos = posS.top();
            posS.pop(); // vi ham move tu nhet trong stack roi
            move(indexArray, oldPostemp1, newPostemp1);
            LuotChoi = !LuotChoi;
            // reset
            click = 0;
        }
        window.draw(sBoard);
        for (int i = 0; i < count; i++)
        {
            sPositive.setPosition(positiveMove[i]);
            window.draw(sPositive);
        }
        for (int i = 0; i < 32; i++)
        {
            window.draw(f[i].s);
        }
        window.display();
        // set oldPostemp == null
        oldPostemp = sf::Vector2f();
    }
}

void PlayWhite(int client_socket)
{
    sf::RenderWindow window(sf::VideoMode(500, 500), "The Chess! Alpha Beta Pruning");
    t1.loadFromFile("figures3.png");
    t2.loadFromFile("board4.png");
    t3.loadFromFile("no.png");

    for (int i = 0; i < 32; i++)
    {
        // Tính toán vị trí cắt cho mỗi quân cờ trên texture
        f[i].s.setTexture(t1);
    }
    sBoard.setTexture(t2);
    sPositive.setTexture(t3);

    Create(); // khoi tao

    bool LuotChoi = true;
    int n = 0, click = 0, count = 0;
    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();
            if (e.type == sf::Event::KeyPressed)
                if (e.key.code == sf::Keyboard::BackSpace)
                {
                    Undo();
                }
            if (e.type == sf::Event::MouseButtonPressed)
                if (e.key.code == sf::Mouse::Left)
                {
                    pos = sf::Mouse::getPosition(window) - sf::Vector2i(offset);
                    click++;
                }
        }
        if (LuotChoi == true)
        {
            if (click == 1)
            {
                bool isMove = false;
                for (int i = 16; i < 32; i++)
                {
                    if (f[i].s.getGlobalBounds().contains(pos.x + offset.x, pos.y + offset.y))
                    {
                        isMove = true;
                        n = i;
                        f[n].s.setColor(sf::Color::Green);
                        oldPos = f[n].s.getPosition();
                    }
                }
                if (!isMove)
                    click = 0;
                else
                {
                    PositiveMoving(n);
                    count = positiveCount;
                    positiveCount = 0;
                }
            }
            if (click == 2)
            {
                f[n].s.setColor(sf::Color::White);
                int x = pos.x / size;
                int y = pos.y / size;
                newPos = sf::Vector2f(x * size, y * size) + offset;
                // chi di chuyen trong vung positiveMove
                for (int i = 0; i < count; i++)
                {
                    if (positiveMove[i] == newPos)
                    {
                        CalculateScore(newPos);
                        if (CheckWin() != 0)
                        {
                            std::cout << "Game Over" << CheckWin() << std::endl;
                            return;
                        }

                        // ...

                        move(n, oldPos, newPos);
                        std::cout << "oldPos: " << oldPos.x << ", " << oldPos.y << std::endl;
                        std::cout << "newPos: " << newPos.x << ", " << newPos.y << std::endl;
                        sendMove(client_socket, n, oldPos, newPos);

                        LuotChoi = !LuotChoi;
                    }
                }
                // reset
                count = 0;
                click = 0;
            }
        }

        else
        {
            auto start = std::chrono::high_resolution_clock::now();
            while (oldPostemp == sf::Vector2f())
            { // Kiểm tra nếu oldPostemp là null
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                if (elapsed.count() >= 30)
                { // Đã chờ đủ 30 giây
                    std::cout << "Timeout waiting for oldPostemp to have a value." << std::endl;
                    return; // Thoát hàm nếu quá thời gian chờ
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            sf::Vector2f oldPostemp1 = convertMove(oldPostemp);
            sf::Vector2f newPostemp1 = convertMove(newPostemp);
            // CalculateScore(newPostemp);
            // if (CheckWin() != 0)
            // {
            //     std::cout << "Game Over" << CheckWin() << std::endl;
            //     return;
            // }
            int c = nS.top();
            nS.pop(); // lay dk thong tin roi xoa di
            oldPos = posS.top();
            posS.pop(); // vi ham move tu nhet trong stack roi
            move(indexArray, oldPostemp1, newPostemp1);
            LuotChoi = !LuotChoi;
            // reset
            click = 0;
        }
        //// draw  ///////
        window.draw(sBoard);
        for (int i = 0; i < count; i++)
        {
            sPositive.setPosition(positiveMove[i]);
            window.draw(sPositive);
        }
        for (int i = 0; i < 32; i++)
        {
            window.draw(f[i].s);
        }
        window.display();
    }
}

void handle_login(int client_socket)
{
    Json::Value jobj;
    jobj["type"] = LOGIN;
    std::string username;
    std::cout << "Username: ";
    std::cin >> username;
    jobj["username"] = username;
    std::string password;
    std::cout << "Password: ";
    std::cin >> password;
    jobj["password"] = password;

    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, jobj);

    send(client_socket, json_string.c_str(), json_string.length(), 0);
}

void handle_register(int client_socket)
{
    Json::Value root;
    root["type"] = REGISTER;

    std::string username;
    std::cout << "Username: ";
    std::cin >> username;
    root["username"] = username;

    std::string password;
    std::cout << "Password: ";
    std::cin >> password;
    root["password"] = password;

    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    // cout << json_string << endl;

    send(client_socket, json_string.c_str(), json_string.length(), 0);
}
void handle_logout(int client_socket)
{
    Json::Value root;
    root["type"] = LOGOUT;
    root["user_id"] = USER_ID;

    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);

    send(client_socket, json_string.c_str(), json_string.length(), 0);
    exit(0);
}

void create_room(int client_socket)
{
    std::cout << "1. Multiplayer" << std::endl;
    std::cout << "2. Singleplayer " << std::endl;
    std::cout << "Let's choose: ";
    int choice;
    std::cin >> choice;

    if (choice == 1)
    {
        // todo
        Json::Value root;
        root["type"] = CREATE_ROOM;
        root["user_id"] = USER_ID;
        Json::StreamWriterBuilder builder;
        const std::string json_string = Json::writeString(builder, root);
        send(client_socket, json_string.c_str(), json_string.length(), 0);
    }
    else
    {
        PlaySingle();
    }
}

void join_room(int client_socket)
{
    //
    Json::Value root;
    root["type"] = JOIN_ROOM;
    root["user_id"] = USER_ID;
    std::cout << "Room id: ";
    int room_id;
    std::cin >> room_id;
    root["room_id"] = room_id;
    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    cout << "json string" << json_string << endl;
    send(client_socket, json_string.c_str(), json_string.length(), 0);
}

void get_room_list(int client_socket)
{
    Json::Value root;
    root["type"] = GET_ROOM_LIST;
    root["user_id"] = USER_ID;
    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    // cout << "json string" << json_string << endl;
    send(client_socket, json_string.c_str(), json_string.length(), 0);
}

void challenge_player(int client_socket)
{
    Json::Value root;
    root["type"] = CHALLENGE;
    root["from_user_id"] = USER_ID;
    std::cout << "Player id: ";
    int player_id;
    std::cin >> player_id;
    root["to_user_id"] = player_id;
    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    // cout << "json string" << json_string << endl;
    send(client_socket, json_string.c_str(), json_string.length(), 0);
}

void chat_player(int client_socket)
{
    Json::Value root;
    root["type"] = CHAT;
    root["from_user_id"] = USER_ID;
    std::cout << "Player id: ";
    int player_id;
    std::cin >> player_id;
    root["to_user_id"] = player_id;
    Json::StreamWriterBuilder builder;
    const std::string json_string = Json::writeString(builder, root);
    // cout << "json string" << json_string << endl;
    send(client_socket, json_string.c_str(), json_string.length(), 0);
}

void *sendMsg(void *arg)
{
    int sockfd = *(int *)arg;
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []
                { return !awaitingResponse; });
        if (is_login == 0)
        {
            std::cout << "CLIENT CHESS!" << std::endl;
            std::cout << "1. Login" << std::endl;
            std::cout << "2. Register" << std::endl;
            std::cout << "3. Exit" << std::endl;
            std::cout << "Your choice: ";

            int choice;
            std::cin >> choice;

            // It's a good idea to check if the input succeeded
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            switch (choice)
            {
            case 1:
                handle_login(sockfd);
                awaitingResponse = true;
                break;
            case 2:
                handle_register(sockfd);
                awaitingResponse = true;
                break;
            case 3:
                close(sockfd);
                exit(0);
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
            }
        }
        else
        {
            std::cout << "Chess" << std::endl;
            std::cout << "1. Create room" << std::endl;
            std::cout << "2. Join room " << std::endl;
            std::cout << "3. View room" << std::endl;
            std::cout << "4. Challenge player" << std::endl;
            std::cout << "5. View rank" << std::endl;
            std::cout << "6. View history" << std::endl;
            std::cout << "7. Logout" << std::endl;
            std::cout << "Your choice: ";

            int choice;
            std::cin >> choice;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            switch (choice)
            {
            case 1:
                cout << USER_ID << endl;
                create_room(sockfd);
                awaitingResponse = true;
                break;
            case 2:
                join_room(sockfd);
                awaitingResponse = true;
                break;
            case 3:
                get_room_list(sockfd);
                awaitingResponse = true;
                break;
            case 4:
                challenge_player(sockfd);
                break;
            // case 5:
            //     // view_challenger(sockfd);
            //     break;
            case 5:
                // view_challenger(sockfd);
                break;
            case 6:
                // view_history(sockfd);
                break;
            case 7:
                handle_logout(sockfd);
                return 0;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
            }
        }
        messageSent = true;
        cv.notify_all();
        lock.unlock();
    }
    pthread_exit(NULL);
}

void *receiveMsg(void *arg)
{
    int sockfd = *(int *)arg;

    while (true)
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []
                { return messageSent; });
        char buffer[1024] = {0};
        int valread = read(sockfd, buffer, 1024);
        if (valread == 0)
        {
            std::cout << "Server disconnected" << std::endl;
            break;
        }
        else if (valread < 0)
        {
            std::cout << "Error receiving message" << std::endl;
            break;
        }
        Json::Value jobj;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;
        bool parsingSuccessful = reader->parse(buffer, buffer + strlen(buffer), &jobj, &errors);
        if (!parsingSuccessful)
        {
            std::cout << "Error parsing JSON" << std::endl;
            continue;
        }
        cout << "jobj" << jobj << endl;
        int type = jobj["type"].asInt();
        if (type == LOGIN)
        {
            int status = jobj["status"].asInt();
            string message = jobj["message"].asCString();
            cout << message << endl;
            if (status == 1)
            {
                is_login = 1;
                USER_ID = jobj["user_id"].asInt();
                USERNAME = jobj["username"].asCString();
                SCORE = jobj["score"].asInt();
                cout << USERNAME << " " << message << endl;
            }
            else
            {
                cout << message << endl;
            }
        }
        else if (type == REGISTER)
        {
            string message = jobj["message"].asCString();
            cout << message << endl;
        }
        else if (type == LOGOUT)
        {
            string message = jobj["message"].asCString();
            cout << message << endl;
        }
        else if (type == CREATE_ROOM)
        {
            int status = jobj["status"].asInt();
            if (status == 1)
            {
                ROOM_ID = jobj["room_id"].asInt();
                cout << "Create room successfully" << endl;
                cout << "Waiting other players to join room " << ROOM_ID << endl;
                PlayWhite(sockfd);
            }
        }
        else if (type == NOTIFI_JOIN_ROOM)
        {
            cout << "type" << type << endl;
            int status = jobj["status"].asInt();
            if (status == 1)
            {
                // int room_id = jobj["room_id"].asInt();
                USER_ID_COMPETIOR = jobj["white_user"]["user_id"].asInt();
                cout << "USER ID COMPETIOR" << USER_ID_COMPETIOR << endl;
                cout << "Join room successfully" << endl;
                PlayBlack(sockfd);
            }
        }

        else if (type == GET_ROOM_LIST)
        {
            int status = jobj["status"].asInt();
            if (status == 1)
            {
                Json::Value room_list = jobj["room_list"];
                for (int i = 0; i < room_list.size(); i++)
                {
                    cout << "Room " << i + 1 << ": " << room_list[i]["room_id"].asInt() << endl;
                }
            }
        }

        else if (type == MOVE)
        {

            // luu lai nuoc di cua doi thu
            int oldPos_x = jobj["oldPos_x"].asInt();
            int oldPos_y = jobj["oldPos_y"].asInt();
            sf::Vector2f oldPostemp = sf::Vector2f(oldPos_x, oldPos_y);
            int newPos_x = jobj["newPos_x"].asInt();
            int newPos_y = jobj["newPos_y"].asInt();
            sf::Vector2f newPostemp = sf::Vector2f(newPos_x, newPos_y);
        }

        else if (type == NOTIFI_CHALLENGE)
        {
            int from_user_id = jobj["from_user_id"].asInt();
            // string from_username = jobj["from_username"].asCString();
            cout << "Player " << from_user_id << " challenge you" << endl;
            cout << "1. Accept" << endl;
            cout << "2. Decline" << endl;
            cout << "Your choice: ";
            int choice;
            cin >> choice;
            if (choice == 1)
            {
                Json::Value root;
                root["type"] = CHALLENGE;
                root["from_user_id"] = from_user_id;
                root["is_accept"] = 1;
                root["to_user_id"] = USER_ID;
                Json::StreamWriterBuilder builder;
                const std::string json_string = Json::writeString(builder, root);
                send(sockfd, json_string.c_str(), json_string.length(), 0);
            }
            else
            {
                Json::Value root;
                root["type"] = CHALLENGE;
                root["from_user_id"] = from_user_id;
                root["is_accept"] = 0;
                root["to_user_id"] = USER_ID;
                Json::StreamWriterBuilder builder;
                const std::string json_string = Json::writeString(builder, root);
                send(sockfd, json_string.c_str(), json_string.length(), 0);
            }
        }

        messageSent = false;
        awaitingResponse = false;
        cv.notify_all();
    }
    pthread_exit(NULL);
}

int main()
{
    pthread_t sendThread, receiveThread;
    pthread_t playThread;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error opening socket" << std::endl;
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error on binding");
        close(sockfd);
        return -1;
    }

    if (pthread_create(&sendThread, NULL, sendMsg, &sockfd))
    {
        perror("Error creating send thread");
        return 1;
    }

    if (pthread_create(&receiveThread, NULL, receiveMsg, &sockfd))
    {
        perror("Error creating receive thread");
        return 1;
    }

    // if (pthread_create(&playThread, NULL, recvPlayThread, &sockfd))
    // {
    //     perror("Error creating play thread");
    //     return 1;
    // }

    if (pthread_join(sendThread, NULL))
    {
        perror("Error joining send thread");
        return 1;
    }

    if (pthread_join(receiveThread, NULL))
    {
        perror("Error joining receive thread");
        return 1;
    }

    // if (pthread_join(playThread, NULL))
    // {
    //     perror("Error joining play thread");
    //     return 1;
    // }

    std::cout << "Both threads completed" << std::endl;
    close(sockfd);

    return 0;
}
