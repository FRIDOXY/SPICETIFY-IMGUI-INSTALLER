#pragma once
#include <string>
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

// Structure d'authentification
struct AuthState {
    bool isAuthenticated = false;
    bool isChecking = false;
    bool showError = false;
    std::string username = "";
    std::string errorMessage = "";
    char usernameBuffer[128] = "";
};

class DiscordAuth {
public:
    // Configuration - REMPLACE PAR TES VRAIES VALEURS
    static const std::string DISCORD_BOT_TOKEN;
    static const std::string DISCORD_SERVER_ID;
    static const std::string DISCORD_INVITE_URL;

    // Vérifie si l'utilisateur est sur le serveur Discord
    static bool CheckUserInServer(const std::string& username, std::string& errorMsg) {
        // MÉTHODE 1 : Code d'accès (plus simple pour commencer)
        if (CheckAccessCode(username, errorMsg)) {
            return true;
        }

        // MÉTHODE 2 : Whitelist locale
        if (CheckLocalWhitelist(username, errorMsg)) {
            return true;
        }

        // MÉTHODE 3 : API Discord (avancé)
        // Décommente cette ligne quand tu auras configuré le bot
        // return CheckMemberInGuild(username, errorMsg);

        // Si rien ne fonctionne
        errorMsg = "Access denied.\nUse code 'SPICETIFY2024' or join Discord!";
        return false;
    }

    // Méthode alternative : Utiliser un système de codes d'accès
    static bool CheckAccessCode(const std::string& code, std::string& errorMsg) {
        const std::string validCodes[] = {
            "SPICETIFY2024",
            "PREMIUM_ACCESS",
            "DISCORD_MEMBER",
            "MANAGER2024",
            "SPOTIFY_PRO"
        };

        // Convertir en majuscules
        std::string upperCode = code;
        for (auto& c : upperCode) c = toupper(c);

        for (const auto& validCode : validCodes) {
            if (upperCode == validCode) {
                return true;  // ← DOIT RETOURNER true ICI
            }
        }

        return false;
    }


private:
    // Vérification via fichier whitelist local
    static bool CheckLocalWhitelist(const std::string& username, std::string& errorMsg) {
        // Lire un fichier whitelist.txt avec les usernames autorisés
        HANDLE hFile = CreateFileA("whitelist.txt", GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            // Si le fichier n'existe pas, ne rien faire (on essaiera autre chose)
            return false;
        }

        char buffer[4096] = { 0 };
        DWORD bytesRead = 0;
        ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        CloseHandle(hFile);

        std::string content(buffer, bytesRead);

        // Chercher le username dans le fichier (ligne par ligne)
        size_t pos = 0;
        std::string line;
        while (pos < content.length()) {
            size_t endLine = content.find('\n', pos);
            if (endLine == std::string::npos) endLine = content.length();

            line = content.substr(pos, endLine - pos);
            // Enlever les espaces et \r
            while (!line.empty() && (line.back() == ' ' || line.back() == '\r' || line.back() == '\n'))
                line.pop_back();

            if (line == username) {
                return true;
            }

            pos = endLine + 1;
        }

        return false;
    }

    // Récupère l'ID utilisateur depuis son username via l'API Discord
    static std::string GetUserIdFromUsername(const std::string& username) {
        // Parse username#discriminator ou nouveau format @username
        std::string searchUsername = username;

        // Recherche de l'utilisateur via l'API Discord Bot
        // On utilise l'endpoint pour lister les membres du serveur
        std::string url = "https://discord.com/api/v10/guilds/" + DISCORD_SERVER_ID + "/members?limit=1000";

        std::string response = MakeDiscordAPIRequest(url);
        if (response.empty()) return "";

        // Parser le JSON pour trouver l'utilisateur
        // Format : [{"user":{"id":"123","username":"name","discriminator":"0001"}}]

        // Recherche simplifiée dans le JSON
        size_t pos = 0;
        while ((pos = response.find("\"username\"", pos)) != std::string::npos) {
            size_t start = response.find("\"", pos + 11) + 1;
            size_t end = response.find("\"", start);
            std::string foundUsername = response.substr(start, end - start);

            if (foundUsername == searchUsername) {
                // Trouver l'ID correspondant
                size_t idPos = response.rfind("\"id\"", pos);
                size_t idStart = response.find("\"", idPos + 5) + 1;
                size_t idEnd = response.find("\"", idStart);
                return response.substr(idStart, idEnd - idStart);
            }
            pos = end;
        }

        return "";
    }

    // Vérifie si un membre est dans le serveur
    static bool CheckMemberInGuild(const std::string& userId, std::string& errorMsg) {
        std::string url = "https://discord.com/api/v10/guilds/" + DISCORD_SERVER_ID + "/members/" + userId;

        std::string response = MakeDiscordAPIRequest(url);

        if (response.empty() || response.find("\"code\":") != std::string::npos) {
            errorMsg = "You are not a member of the Discord server!\nJoin to get access.";
            return false;
        }

        // Si on reçoit des données membre, c'est OK
        if (response.find("\"user\"") != std::string::npos) {
            return true;
        }

        errorMsg = "Verification failed. Try again.";
        return false;
    }

    // Fait une requête HTTP à l'API Discord avec authentification
    static std::string MakeDiscordAPIRequest(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("SpicetifyManager/1.0",
            INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

        if (!hInternet) return "";

        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(),
            NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);

        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }

        // Ajouter le header Authorization avec le token du bot
        std::string authHeader = "Authorization: Bot " + DISCORD_BOT_TOKEN;
        HttpAddRequestHeadersA(hConnect, authHeader.c_str(), authHeader.length(),
            HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);

        char buffer[16384] = { 0 };
        DWORD bytesRead = 0;
        std::string fullResponse;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            fullResponse.append(buffer, bytesRead);
            memset(buffer, 0, sizeof(buffer));
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        return fullResponse;
    }
};

// Configuration à remplir
const std::string DiscordAuth::DISCORD_BOT_TOKEN = "MTQzOTI1MTE2OTgwODc0ODYxNA.GBtwMz.OVWbq2NjuZf9VQG3_Zh23NqLEGMTeQlnjk6R0I";  // ← Colle ton token ici
const std::string DiscordAuth::DISCORD_SERVER_ID = "836135567518466078";  // ← Colle l'ID de ton serveur ici
const std::string DiscordAuth::DISCORD_INVITE_URL = "https://discord.gg/PjvMxkh5bZ";  // ← Ton lien d'invitation



