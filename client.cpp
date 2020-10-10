#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "json.hpp"
#include "helpers.h"
#include "requests.h"
#include "buffer.h"

using namespace std;
using json = nlohmann::json;
bool loged = false; // arata daca utilizatorul este logat sau nu
bool accessLib = false; // arata daca utilizatorul are access
int sockfd;
char host[] = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
char reqType[] = "application/json";
string cookie;
string token;
//verifica daca sctringul primit este numar
//util pentru id sau page_count
bool isNumber(string number)
{
    string::const_iterator it = number.begin();
    while (it != number.end() && std::isdigit(*it))
        ++it;
    return !number.empty() && it == number.end();
}
//facem un json pentru noua carte care trebuie adaugata
bool makeNewBook(char *data)
{
    printf("title=");
    string title;
    getline(cin, title);
    printf("author=");
    string author;
    getline(cin, author);
    printf("genre=");
    string genre;
    getline(cin, genre);
    printf("publisher=");
    string publisher;
    getline(cin, publisher);
    printf("page_count=");
    string page_count;
    getline(cin, page_count);
    if (!isNumber(page_count))
    {
        printf("Please provide a number\n");
        return false;
    }
    json book = {
        {"title", title.c_str()},
        {"author", author.c_str()},
        {"genre", genre.c_str()},
        {"page_count", page_count.c_str()},
        {"publisher", publisher.c_str()}};
    string toSend = book.dump();
    char toSendMsg[strlen(toSend.c_str())];
    strcpy(data, toSend.c_str());
    return true;
}

void registerUser()
{
    //prompturile pentru utilizator
    printf("register\n");
    printf("username=");
    string username;
    getline(cin, username);
    printf("password=");
    string password;
    getline(cin, password);
    //deschidem conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    //facem jsonul
    json regist = {
        {"username", username.c_str()},
        {"password", password.c_str()}};

    string toSend = regist.dump();
    char toSendMsg[strlen(toSend.c_str())];
    strcpy(toSendMsg, toSend.c_str());
    char *message;
    char *response;
    //si transmitem mesajul
    message = compute_post_request(host, "/api/v1/tema/auth/register", reqType, toSendMsg, NULL, false);
    //puts(message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    //printf("%s\n", response);
    //inchidem conexiunea
    close_connection(sockfd);
}

void loginUser()
{
    //verificare daca utilizatorul este deja logat
    if (loged)
    {
        printf("You are already loged in\n");
        return;
    }
    //in rest este foarte asemanator cu register
    printf("login\n");
    printf("username=");
    string username;
    getline(cin, username);
    printf("password=");
    string password;
    getline(cin, password);

    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
    json regist = {
        {"username", username.c_str()},
        {"password", password.c_str()}};

    string toSend = regist.dump();
    char toSendMsg[strlen(toSend.c_str())];
    strcpy(toSendMsg, toSend.c_str());
    char *message;
    char *response;
    message = compute_post_request(host, "/api/v1/tema/auth/login", reqType, toSendMsg, NULL, false);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    //puts(response);
    // ce este in plus fata de register este extragerea si memorarea cookieului
    if (strstr(response, "OK"))
    {
        loged = true;
        char *temp_cookie = strtok(strstr(response, "connect.sid"), ";");
        printf("%s\n", temp_cookie);
        cookie = string(temp_cookie);
    }
    else
    {
        printf("Wrong Username/Password\n");
    }
    close_connection(sockfd);
}

void logoutUser()
{
    if (loged)
    {
        char *message;
        char *response;
        char temp_cookie[strlen(cookie.c_str())];
        strcpy(temp_cookie, cookie.c_str());
        sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
        message = compute_get_request(host, "/api/v1/tema/auth/logout", NULL, temp_cookie, false, false);
        send_to_server(sockfd, message);

        //puts(message);
        response = receive_from_server(sockfd);
        //printf("%s\n", response);
        //resetam campurile de acces
        loged = false;
        accessLib = false;

        close_connection(sockfd);
    }
    else
    {
        printf("Please login first\n");
        return;
    }
}
void getAccessLib()
{
    if (loged)
    {
        char *message;
        char *response;
        char temp_cookie[strlen(cookie.c_str())];
        strcpy(temp_cookie, cookie.c_str());
        sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
        message = compute_get_request(host, "/api/v1/tema/library/access", NULL, temp_cookie, false, false);
        send_to_server(sockfd, message);
        //puts(message);
        response = receive_from_server(sockfd);
        //puts(response);

        close_connection(sockfd);
        //extragen tokenul
        if (strstr(response, "OK"))
        {
            accessLib = true;
            //adaugam un + 8 ca sa sarim peste partea cu "toke":"
            char *temp_cookie = strtok(strstr(response, "token\":\"") + 8, "\"");
            printf("%s\n", temp_cookie);
            token = string(temp_cookie);
        }
        else
        {
            printf("Error getting access, please try again.\n");
        }
    }
    else
    {
        printf("Please login first\n");
        return;
    }
}

void getBooks()
{
    if (loged)
    {
        if (accessLib)
        {
            char *message;
            char *response;
            char temp_cookie[strlen(token.c_str())];
            strcpy(temp_cookie, token.c_str());
            sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/library/books", NULL, temp_cookie, true, false);
            send_to_server(sockfd, message);
            //puts(message);
            response = receive_from_server(sockfd);
            //puts(response);
            close_connection(sockfd);

            if (strstr(response, "OK"))
            {
                //afisam toata colectia de carti 
                char *temp_cookie = strtok(strstr(response, "["), "]");
                printf("%s]\n", temp_cookie);
            }
            else
            {
                printf("Error getting books, please try again\n");
            }
        }
        else
        {
            printf("Please get access first\n");
            return;
        }
    }
    else
    {
        printf("Please login first\n");
        return;
    }
}
//folosim aceasi metoda si pentru delete deoarece mesajele
//transmise la server sunt foarte asemanatoare
//parametrul are scopul de a lasa fuctia compute_get_request sa
//stie daca mesajul este unul de get sau delete
void getBook(bool del)
{
    if (loged)
    {
        if (accessLib)
        {
            printf("id=");
            string id;
            getline(cin, id);
            //veficam daca id ul este numar 
            if (!isNumber(id))
            {
                printf("Please provide a number\n");
                return;
            }
            char *message;
            char *response;
            char temp_cookie[strlen(token.c_str())];
            strcpy(temp_cookie, token.c_str());
            //stabilim conexiunea
            sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
            char url[27 + strlen(id.c_str())];
            //concateam id ul la url
            sprintf(url, "/api/v1/tema/library/books/%s", id.c_str());
            message = compute_get_request(host, url, NULL, temp_cookie, true, del);
            send_to_server(sockfd, message);
            //puts(message);
            response = receive_from_server(sockfd);
            //puts(response);
            close_connection(sockfd);

            if (strstr(response, "OK"))
            {
                if (!del)
                {
                    char *temp_cookie = strtok(strstr(response, "["), "]");
                    printf("%s]\n", temp_cookie);
                }
                else
                {
                    printf("Book deleted\n");
                }
            }
            else
            {
                printf("Error getting book, check id, please try again\n");
            }
        }
        else
        {
            printf("Please get access first\n");
            return;
        }
    }
    else
    {
        printf("Please login first\n");
        return;
    }
}
void addBook()
{
    if (loged)
    {
        if (accessLib)
        {
            //memoreaza dump ul de la json ul creat
            char data[1500];
            if (!makeNewBook(data))
            {
                printf("Please provide a correct number for page count\n");
                return;
            }
            char *message;
            char *response;
            char temp_cookie[strlen(token.c_str())];
            strcpy(temp_cookie, token.c_str());
            sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(host, "/api/v1/tema/library/books", reqType, data, temp_cookie, true);
            send_to_server(sockfd, message);
            //puts(message);
            response = receive_from_server(sockfd);
            //puts(response);
            close_connection(sockfd);

            if (strstr(response, "OK"))
            {
                printf("Created book\n");
            }
            else
            {
                printf("Error adding book, please try again\n");
            }
        }
        else
        {
            printf("Please get access first\n");
            return;
        }
    }
    else
    {
        printf("Please login first\n");
        return;
    }
}

int main(int argc, char *argv[])
{
    string command = "ok";
    while (command != "exit")
    {
        getline(cin, command);

        if (command == "register")
        {
            registerUser();
            continue;
        }
        if (command == "login")
        {
            loginUser();
            continue;
        }
        if (command == "logout")
        {
            logoutUser();
            continue;
        }
        if (command == "enter_library")
        {
            getAccessLib();
            continue;
        }
        if (command == "get_books")
        {
            getBooks();
            continue;
        }
        if (command == "get_book")
        {
            getBook(false);
            continue;
        }
        if (command == "delete_book")
        {
            getBook(true);
            continue;
        }
        if (command == "add_book")
        {
            addBook();
            continue;
        }
    }
    close_connection(sockfd);
    return 0;
}