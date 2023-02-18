#include "Command.hpp"

void    CommandPASS::execute(Client *client, std::vector<std::string> arguments)
{
    if (client->isRegistered())
    {
        client->reply(ERR_ALREADYREGISTERED(client->getNick()));
        return ;
    }
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "PASS"));
        return ;
    }
    if (_server->getPass() != arguments[0].substr(arguments[0][0] == ':' ? 1 : 0))
    {
        client->reply(ERR_PASSWDMISMATCH(client->getNick()));
        return ;
    }
    client->unlockPasswd();
    client->registering();
}

void    CommandNICK::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NONICKNAMEGIVEN(client->getNick()));
        return ;
    }
    if (!client->hasPasswd())
    {
        client->reply(ERR_ALREADYREGISTERED(client->getNick()));
        return ;
    }
    std::string nickname = arguments[0];
    if (!is_valid(nickname))
    {
        client->reply(ERR_ERRONEUSNICKNAME(client->getNick(), nickname));
        return ;
    }
    Client* client_avel = _server->getClient(nickname);
    if (client_avel &&  client_avel != client)
    {
        client->reply(ERR_NICKNAMEINUSE(client->getNick(), nickname));
        return ;
    }
    _server->setUser(client, nickname, client->getFd());
    client->setNick(nickname);
    if (!client->isRegistered())
        client->registering();
}

void    CommandUSER::execute(Client *client, std::vector<std::string> arguments)
{
    if (client->isRegistered() || !client->hasPasswd())
    {
        client->reply(ERR_ALREADYREGISTERED(client->getNick()));
        return ;
    }
    if (arguments.size() < 4)
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "USER"));
        return ;
    }
    std::string username = arguments[0];
    std::string realname = arguments[3].substr(arguments[3][0] == ':' ? 1 : 0);
    for (size_t i = 4; i < arguments.size(); ++i)
        realname.append(" " + arguments[i]);
    
    client->init(username, realname);
    client->registering();
}

void    CommandPRIVMSG::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "PRIVMSG"));
        return ;
    }
    if (arguments.size() < 2)
    {
        client->reply(ERR_NOTEXTTOSEND(client->getNick()));
        return ;
    }
    std::string target = arguments[0];
    std::string message = arguments[1].substr(arguments[1][0] == ':' ? 1 : 0);
    for (size_t i = 2; i < arguments.size(); ++i)
        message.append(" " + arguments[i]);
    if (target[0] == '#' || target[0] == '&')
    {
        Channel* channel = _server->getChannel(target);
        if (!channel)
        {
            client->reply(ERR_NOSUCHNICK(client->getNick(), target));
            return ;
        }
        if (!channel->isAvelabel(client))
        {
            client->reply(ERR_CANNOTSENDTOCHAN(client->getNick(), target));
            return ;
        }
        channel->sending(client, message, "PRIVMSG");
        return ;
    }

    Client* user = _server->getClient(target);
    if (!user)
    {
        client->reply(ERR_NOSUCHNICK(client->getNick(), target));
        return ;
    }
    user->sending(RPL_MSG(client->getPrefix(), "PRIVMSG", target, message));
}

void    CommandNOTICE::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "NOTICE"));
        return ;
    }
    if (arguments.size() < 2)
    {
        client->reply(ERR_NOTEXTTOSEND(client->getNick()));
        return ;
    }
    std::string target = arguments[0];
    std::string message = arguments[1].substr(arguments[1][0] == ':' ? 1 : 0);
    for (size_t i = 2; i < arguments.size(); ++i)
        message.append(" " + arguments[i]);
    if (target[0] == '#')
    {
        Channel* channel = _server->getChannel(target);
        if (!channel)
        {
            client->reply(ERR_NOSUCHNICK(client->getNick(), target));
            return ;
        }
        if (!channel->isAvelabel(client))
        {
            client->reply(ERR_CANNOTSENDTOCHAN(client->getNick(), target));
            return ;
        }
        channel->sending(client, message, "NOTICE");
        return ;
    }

    Client* user = _server->getClient(target);
    if (!user)
    {
        client->reply(ERR_NOSUCHNICK(client->getNick(), target));
        return ;
    }
    user->sending(RPL_MSG(client->getPrefix(), "NOTICE", target, message));
}

void    CommandCAP::execute(Client *client, std::vector<std::string> arguments)
{
    (void)client; (void)arguments;
}

void    CommandPING::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "PING"));
        return ;
    }
    client->sending(RPL_PING(client->getPrefix(), arguments[0]));
}

void    CommandPONG::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "PONG"));
        return ;
    }
    client->sending(RPL_PING(client->getPrefix(), arguments[0]));
}

void    CommandJOIN::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "JOIN"));
        return ;
    }
    if (arguments.size() == 1 && arguments[0] == "0")
    {
        client->leaveChannel(0);
        return ;
    }
    std::string channel_name = arguments[0];
    std::string password = arguments.size() > 1 ? arguments[1] : "";
    std::map<std::string, std::string> ch = strTransMap(channel_name, password);
    for (std::map<std::string, std::string>::iterator it = ch.begin(); it != ch.end(); ++it) {
        channel_name = it->first;
        password = it->second;
        if (channel_name[0] != '#' && channel_name[0] != '&')
        {
            client->reply(ERR_BADCHANMASK(client->getNick(), channel_name));
            return ;
        }
        Channel* channel = _server->getChannel(channel_name);
        if (!channel)
            channel = _server->addChannel(channel_name, password);
        else if (channel->usersSize() == 0)
            channel->setKey(password);
        if (channel->getKey() != "" && channel->getKey() != password)
        {
            client->reply(ERR_BADCHANNELKEY(client->getNick(), channel_name));
            return ;
        }
        client->joinChannel(channel);
    }
}

void    CommandPART::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "PART"));
        return ;
    }
    std::string channel_name = arguments[0];

    Channel* channel = _server->getChannel(channel_name);
    if (!channel)
    {
        client->reply(ERR_NOSUCHCHANNEL(client->getNick(), channel_name));
        return ;
    }
    else if (!channel->isAvelabel(client))
    {
        client->reply(ERR_NOTONCHANNEL(client->getNick(), channel_name));
        return ;
    }
    channel->part(client);
    client->leaveChannel(channel);
}

void    CommandKICK::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty() || arguments.size() < 2)
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "KICK"));
        return ;
    }
    std::string channel_name = arguments[0];

    Channel* channel = _server->getChannel(channel_name);
    if (!channel)
    {
        client->reply(ERR_NOSUCHCHANNEL(client->getNick(), channel_name));
        return ;
    }
    else if (!channel->isAvelabel(client))
    {
        client->reply(ERR_NOTONCHANNEL(client->getNick(), channel_name));
        return ;
    }
    if (!(channel->isAdmin(client)))
    {
        client->reply(ERR_CHANOPRIVSNEEDED(client->getNick(), channel_name));
        return ;
    }
    std::string user_name = arguments[1];

    Client* user = _server->getClient(user_name);
    if (!user || !channel->search_user(user))
    {
        client->reply(ERR_USERNOTINCHANNEL(client->getNick(), user_name, channel_name));
        return ;
    }
    std::string reason = "No reason specified.";
    if (arguments.size() > 2)
    {
        reason = arguments[2].substr(arguments[2][0] == ':' ? 1 : 0);
        for (size_t i = 3; i < arguments.size(); ++i)
            reason.append(" " + arguments[i]);
    }
    channel->kick(user, reason);
    user->leaveChannel(channel);
}

void    CommandQUIT::execute(Client *client, std::vector<std::string> arguments)
{
    std::string reason;
    if (arguments.size())
    {
        reason = arguments[0].substr(arguments[0][0] == ':' ? 1 : 0);
        for (size_t i = 1; i < arguments.size(); ++i)
            reason.append(" " + arguments[i]);
    }
    client->quit = true;
    client->sending(RPL_QUIT(client->getPrefix(), reason));
}

void    CommandMODE::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "MODE"));
        return ;
    }
    std::string channel_name = arguments[0];

    Channel* channel = _server->getChannel(channel_name);
    if (!channel)
    {
        client->reply(ERR_NOSUCHCHANNEL(client->getNick(), channel_name));
        return ;
    }
    else if (!channel->isAvelabel(client))
    {
        client->reply(ERR_NOTONCHANNEL(client->getNick(), channel_name));
        return ;
    }
    if (arguments.size() == 1)
    {
        if (!(channel->getKey() == ""))
            client->sending(RPL_MODE(client->getPrefix(), channel_name, "+k " + channel->getKey()));
        return ;
    }
    std::string mode = arguments[1];
    if (arguments.size() > 2)
    {
        std::string key = arguments[2];
        if (!(channel->isAdmin(client)))
            client->reply(ERR_CHANOPRIVSNEEDED(client->getNick(), channel_name));
        else if (mode == "+k")
        {
            client->sending(RPL_MODE(client->getPrefix(), channel_name, "+k " + key));
            channel->setKey(key);
        }
        else if (mode == "-k" && channel->getKey() == key)
        {
            client->sending(RPL_MODE(client->getPrefix(), channel_name, "-k "));
            channel->setKey("");
        }
    }
}

void    CommandWHO::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty())
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "WHO"));
        return ;
    }
    std::string target = arguments[0];

    if (target[0] == '#' || target[0] == '&')
    {
        Channel* channel = _server->getChannel(target);
        if (!channel)
        {
            client->reply(ERR_NOSUCHNICK(client->getNick(), target));
            return ;
        }
        if (!channel->isAvelabel(client))
        {
            client->reply(ERR_NOTONCHANNEL(client->getNick(), target));
            return ;
        }
        channel->whoReply(client);
        return ;
    }
    Client* user = _server->getClient(target);
    if (!user)
    {
        client->reply(ERR_NOSUCHNICK(client->getNick(), target));
        return ;
    }
    client->sending(RPL_WHOREPLY(client->getNick(), "*", user->getUser(), user->getHost(), user->getNick(), "H", user->getReal()));
    client->sending(RPL_ENDOFWHO(client->getNick(), target));
}

void    CommandINVITE::execute(Client *client, std::vector<std::string> arguments)
{
    if (arguments.empty() || arguments.size() < 2)
    {
        client->reply(ERR_NEEDMOREPARAMS(client->getNick(), "WHO"));
        return ;
    }
    std::string user_name = arguments[0];
    Client* user = _server->getClient(user_name);
    if (!user)
    {
        client->reply(ERR_NOSUCHNICK(client->getNick(), user_name));
        return ;
    }
    std::string channel_name = arguments[1];
    Channel* channel = _server->getChannel(channel_name);
    if (!channel)
    {
        client->reply(ERR_NOSUCHNICK(client->getNick(), channel_name));
        return ;
    }
    if (!channel->isAvelabel(client))
    {
        client->reply(ERR_NOTONCHANNEL(client->getNick(), channel_name));
        return ;
    }
    if (channel->isAvelabel(user))
    {
        client->reply(ERR_USERONCHANNEL(client->getNick(), user_name, channel_name));
        return ;
    }
    user->sending(RPL_INVITE(client->getPrefix(), user_name, channel_name));
    client->reply(RPL_INVITING(client->getNick(), user_name, channel_name));


}