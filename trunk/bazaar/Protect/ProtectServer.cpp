#include "ProtectServer.h"

NAMESPACE_UPP

// email validation helper
bool ValidateEMail(const char *address)
{
	int					count = 0;
	const char			*c, *domain;
	static const char	*rfc822_specials = "()<>@,;:\\\"[]";
	
	// first we validate the name portion (name@domain)
	for (c = address;  *c;  c++)
	{
		if (*c == '\"' && (c == address || *(c - 1) == '.' || *(c - 1) == '\"'))
		{
			while (*++c)
			{
				if (*c == '\"')
					break;
				if (*c == '\\' && (*++c == ' '))
					continue;
				if (*c < ' ' || *c >= 127)
					return false;
			}
			if (!*c++)
				return false;
			if (*c == '@')
				break;
			if (*c != '.')
				return false;
			continue;
		}
		if (*c == '@')
			break;
		if (*c <= ' ' || *c >= 127)
			return false;
		if (strchr(rfc822_specials, *c))
			return false;
	}
	if (c == address || *(c - 1) == '.')
		return false;
	
	// next we validate the domain portion (name@domain)
	if (!*(domain = ++c))
		return false;
	do
	{
		if (*c == '.')
		{
			if (c == domain || *(c - 1) == '.')
				return false;
			count++;
		}
		if (*c <= ' ' || *c >= 127)
			return false;
		if (strchr(rfc822_specials, *c))
			return false;
	}
	while (*++c);
	return (count >= 1);
}

// constructor - defaults to port 8787 and connection timeout of 5 minutes (300 sec.)
ProtectServer::ProtectServer(int port) : ScgiServer(port)
{
	// setup default cypher
	cypher = new Snow2;
	
	// client timeout defaults to 300 seconds (5 minutes)
	idCleanupTime = 300;

	// database cleanup time -- default to 30 days (2592000 seconds)
	// used to clean up registrations with no activation on database
	dbCleanupTime = 2592000;

	// initializes last cleanups times to now()
	lastIDCleanupTime = GetSysTime();
	lastDBCleanupTime = GetSysTime();
}

// generates an unique activation key composed by 20 random uppercase chars
String ProtectServer::GenerateActivationKey(void)
{
	String key;
	for(int i = 0; i < 20; i++)
		key << (char)(Random('Z' - 'A') + 'A');
	return key;
}

// executes database cleanup
void ProtectServer::DBCleanup(void)
{
	// @@@ TO DO !!!
	Cerr() << "DB Cleanup :: TO DO\n";
}

// polls clients and remove those which connection has expired
void ProtectServer::CheckExpiredClients(void)
{
	Time curTime = GetSysTime();
	for(int iClient = clientLinks.GetCount() - 1; iClient >= 0; iClient--)
		if(clientLinks[iClient].time < curTime)
		{
			String eMail = clientLinks[iClient].eMail;
			bool connected = clientLinks[iClient].connected;
			clientLinks.Remove(iClient);
			if(connected && registeredConnections.Find(eMail) >= 0)
			{
				registeredConnections.Get(eMail)--;
				if(registeredConnections.Get(eMail) <= 0)
					registeredConnections.RemoveKey(eMail);
			}
		}
}
		
// checks client list to see if a client is connected
// and refresh its expire time on the way
bool ProtectServer::IsClientConnected(dword clientID, String const &eMail)
{
	int iClient = clientLinks.Find(clientID);
	if(iClient >= 0 && clientLinks[iClient].connected && clientLinks[iClient].eMail == eMail)
	{
		clientLinks[iClient].time = GetSysTime() + idCleanupTime;
		return true;
	}
	return false;
}

// connects a client -- returns previous clientID or a new one if expired
// doesn't check anything, that one must be done elsewhere
dword ProtectServer::ConnectClient(String const &eMail, dword id)
{
	// try to fetch id from active pool
	int iClient = clientLinks.Find(id);
	
	// if not found in pool or ID allocated to another user, allocate a new one
	if(iClient < 0 || clientLinks[iClient].eMail != eMail)
	{
		// mark id as invalid
		iClient = -1;
		
		// generate a new id
		do
		{
			id = Random();
		}
		while(id == 0 || id == -1 || clientLinks.Find(id) >= 0);
	}
	
	// adds to clientLinks and to pool if needed, otherwise update record
	if(iClient < 0)
	{
		// a new client ID was generated, add it to pool, by now as non-connected one
		clientLinks.Add(id, TimeMail(GetSysTime() + idCleanupTime, eMail, false));
		iClient = clientLinks.GetCount() - 1;
	}
	else
	{
		// no new ID was generated, client probably exited without disconnecting
		//  just refresh its time and mark it as connected
		clientLinks[iClient].time = GetSysTime() + idCleanupTime;
	}

	// if client marked as non connected, update connections count
	if(!clientLinks[iClient].connected)
	{
		clientLinks[iClient].connected = true;
		int i = registeredConnections.Find(eMail);
		if(i >= 0)
			registeredConnections[i]++;
		else
			registeredConnections.Add(eMail, 1);
	}
	
	return id;
}
		
// disconnects a client -- leaves id on pool up to expiration
// updates count of clients connected by email
void ProtectServer::DisconnectClient(dword clientID)
{
	int i = clientLinks.Find(clientID);
	if(i >= 0)
	{
		String eMail = clientLinks[i].eMail;
		clientLinks[i].connected = false;
		i = registeredConnections.Find(eMail);
		if(i >= 0)
		{
			registeredConnections[i]--;
			if(registeredConnections[i] <= 0)
				registeredConnections.Remove(i);
		}
	}
}
		

void ProtectServer::OnAccepted()
{
	Cout() << "Accepted connection from client " << FormatIP(clientIP) << "\n";
}

void ProtectServer::OnRequest()
{
	int err = 0;
	VectorMap<String, Value> data;
	VectorMap<String, Value> results;
	String IV;
	
	// we handle just POST requests, GET not allowed
	if(HasPostData())
	{
		// all requests besides PROTECT_ACTIVATE go through post
		if(post.Find("IV") < 0)
			err = PROTECT_MISSING_IV;
		else
		{
			IV = ScanHexString(post.GetString("IV"));
			if(post.Find("DATA") < 0)
				err = PROTECT_MISSING_DATA;
			else
			{
				cypher->SetKey(communicationKey, IV);
				String decoded = (*cypher)(ScanHexString(post.GetString("DATA")));
				LoadFromXML(data, decoded);
			}
		}
	}
	else
		err = PROTECT_BAD_REQUEST;


	// Get request reason and process it
	// supports following reasons :
	//		CONNECT			establish connection to server
	//		DISCONNECT		frees server connection
	//		REFRESH			refreshes server connection (to restart timeout)
	//		GETKEY			gets application key
	//		REGISTER		registers app for timed demo OR re-request activation code if already registered
	//		GETLICENSEINFO	gets info about license (name, expiration date, app version....)
	if(!err && data.Find("REASON") < 0)
		err = PROTECT_MISSING_REASON;
	if(!err)
		results = ProcessRequest(data.Get("REASON"), data);
	
	// if previous error, sets it into result packet
	if(err)
	{
		results.Add("ERROR", err);
		results.Add("ERRORMSG", ProtectMessage(err));
	}
	else if(results.Find("ERROR") >= 0)
		results.Add("ERRORMSG", ProtectMessage(results.Get("ERROR")));

	// encodes results and send back to client
	clientSock.Write("Content-Type: text/plain\r\n\r\n");
	cypher->SetKey(communicationKey);
	clientSock.Write("IV=" + HexString(cypher->GetNonce()) + "\r\n");
	String encoded = HexString((*cypher)(StoreAsXML(results, "ProtectServer")));
	clientSock.Write("DATA=" + encoded + "\r\n");
}

void ProtectServer::OnClosed()
{
	// do client and db cleanup if related times are expired
	// NOTE : db cleanup is time consuming -- DON'T put a short time !!
	// it would be maybe better to put it inside a separate thread/process ??
	Time now = GetSysTime();
	if(now - lastIDCleanupTime > idCleanupTime)
		CheckExpiredClients();
	if(now - lastDBCleanupTime > dbCleanupTime)
		DBCleanup();

	Cout() << "Connection with " << FormatIP(clientIP) << " closed\n";
}

// process client request
// takes a VectorMap<String, Value> on input from client
// produces a response VectorMap<String, Value> to be returned
VectorMap<String, Value> ProtectServer::ProcessRequest(int reason, VectorMap<String, Value> const &v)
{
	VectorMap<String, Value> res;
	dword clientID;
	VectorMap<String, Value> userRec;
	String eMail;
	String activationKey;
	int numConnections;
	
	switch(reason)
	{
		case PROTECT_REGISTER:
			{
				// we need an eMail to register
				if(v.Find("EMAIL") < 0)
				{
					res.Add("ERROR", PROTECT_MISSING_EMAIL);
					return res;
				}
				eMail = v.Get("EMAIL");
				if(!ValidateEMail(~eMail))
				{
					res.Add("ERROR", PROTECT_INVALID_EMAIL);
					return res;
				}
				
				// avoid hacking of source packet stripping eventually
				// added licenseinfo, expiration, etc
				VectorMap<String, Value>vs(v, 1);
				vs.RemoveKey("LICENSES");
				vs.RemoveKey("EXPIRATION");
				vs.RemoveKey("ACTIVATIONKEY");
				vs.RemoveKey("REGISTRATIONDATE");
				vs.RemoveKey("ACTIVATED");
				vs.RemoveKey("ACTIVATIONDATE");
				vs.RemoveKey("LASTCONNECTIONDATE");
				vs.RemoveKey("TOTALCONNECTIONS");
				vs.RemoveKey("TOTALCONNECTIONTIME");
		
				// try to get user record from database, if any
				userRec = db.Get(eMail);
				
				// if not found, new registration
				if(!userRec.GetCount())
				{
					// generates activation key from email
					vs.Add("ACTIVATIONKEY", GenerateActivationKey());
					vs.Add("REGISTRATIONDATE", GetSysTime());
					vs.Add("ACTIVATED", false);
					vs.Add("ACTIVATIONDATE", GetSysTime());			// dummy, maybe not needed now
					vs.Add("LASTCONNECTIONDATE", GetSysTime());		// dummy, maybe not needed now
					vs.Add("TOTALCONNECTIONS", 0);
					vs.Add("TOTALCONNECTIONTIME", 0);
					
					// add to database
					db.Set(vs);
					
					// send the activation mail to user
					userRec = db.Get(eMail);
					if(SendActivationMail(userRec))
						res.Add("STATUS", "ACTIVATION RESENT");
					else
						res.Add("ERROR", PROTECT_MAIL_SEND_ERROR);
				}
				// otherwise we shall check if license expired
				else if(userRec.Get("EXPIRATION") <= GetSysTime())
				{
					// signals expiration and leave
					res.Add("ERROR", PROTECT_MAIL_ALREADY_USED);
				}
				// otherwise, we shall send activation mail again
				// it may be same user on another machine
				else
				{
					// already registered but still not activated
					// resend activation mail
					if(SendActivationMail(userRec))
						res.Add("STATUS", "ACTIVATION RESENT");
					else
						res.Add("ERROR", PROTECT_MAIL_SEND_ERROR);
				}
			}
			break;
		case PROTECT_CONNECT:
		{
			// we need email to connect
			if(v.Find("EMAIL") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_EMAIL);
				return res;
			}
			eMail = v.Get("EMAIL");
			
			// we need previous clientID to connect, (or -1 if none)
			if(v.Find("CLIENTID") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_CLIENTID);
				return res;
			}
			clientID = (int)v.Get("CLIENTID");
			
			// if already connected, just return ok
			if(IsClientConnected(clientID, eMail))
				return res;
			
			// get the user data from database
			userRec = db.Get(eMail);

			// if no mail data, product is unregistered
			if(!userRec.GetCount())
			{
				res.Add("ERROR", PROTECT_UNREGISTERED);
				return res;
			}
			
			// product registered, we check for activation key
			if(v.Find("ACTIVATIONKEY") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_ACTIVATIONKEY);
				SendActivationMail(userRec);
				return res;
			}
			
			// check for activation key correctness
			if(v.Get("ACTIVATIONKEY") != userRec.Get("ACTIVATIONKEY"))
			{
				res.Add("ERROR", PROTECT_BAD_ACTIVATIONKEY);
				SendActivationMail(userRec);
				return res;
			}
			
			// ok, product registered and activated... is license expired ?
			if(userRec.Get("EXPIRATION") < GetSysTime())
			{
				res.Add("ERROR", PROTECT_LICENSE_EXPIRED);
				return res;
			}
			
			// all ok, we shall finally check if number of licenses has overrun
			// first, we get number of current connections with same email
			numConnections = registeredConnections.Find(eMail);
			if(numConnections < 0)
				numConnections = 0;
			else
				numConnections = registeredConnections[numConnections];
			if(numConnections >= (int)userRec.Get("LICENSES"))
			{
				res.Add("ERROR", PROTECT_LICENSES_NUMBER_EXCEEDED);
				return res;
			}
			
			// all OK, connect client and return connection ID
			clientID = ConnectClient(eMail, clientID);
			res.Add("CLIENTID", (int)clientID);
			
			// mark product as activated in database, if needed
			if(!userRec.Get("ACTIVATED"))
			{
				userRec.Get("ACTIVATED") = true;
				userRec.Get("ACTIVATIONDATE") = GetSysTime();
			}
			
			// updates last connection date in database
			userRec.Get("LASTCONNECTIONDATE") = GetSysTime();
			int connections = userRec.Get("TOTALCONNECTIONS");
			userRec.Get("TOTALCONNECTIONS") = connections + 1;
			
			// updates database record
			db.Set(userRec);
			
			break;
		}
			
		case PROTECT_DISCONNECT :
		{
			// we need email to disconnect
			if(v.Find("EMAIL") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_EMAIL);
				return res;
			}
			eMail = v.Get("EMAIL");

			// we need the conneciton ID
			if(v.Find("CLIENTID") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_CLIENTID);
				return res;
			}
			clientID = (int)v.Get("CLIENTID");
			
			// check whether client ID is in pool
			// otherwise it may be not connected or expired
			if(!IsClientConnected(clientID, eMail))
				return res;
			
			// updates database connection time for user
			// warning --- there's a caveat if client side crashes without disconnecting
			// the total connection time won't be updated - it would make few sense anyways
			userRec = db.Get(eMail);
			int time = GetSysTime() - (Time)userRec.Get("LASTCONNECTIONDATE");
			time += (int)userRec.Get("TOTALCONNECTIONTIME");
			userRec.Get("TOTALCONNECTIONTIME") = time;
			db.Set(userRec);
			
			DisconnectClient(clientID);
			break;
		}
			
		case PROTECT_REFRESH :
			// we need email to refresh
			if(v.Find("EMAIL") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_EMAIL);
				return res;
			}
			eMail = v.Get("EMAIL");

			// we need the conneciton ID
			if(v.Find("CLIENTID") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_CLIENTID);
				return res;
			}
			clientID = (int)v.Get("CLIENTID");

			if(!IsClientConnected(clientID, eMail))
			{
				res.Add("ERROR", PROTECT_NOT_CONNECTED);
				return res;
			}
			break;
			
		case PROTECT_GETLICENSEKEY:
			// we need email
			if(v.Find("EMAIL") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_EMAIL);
				return res;
			}
			eMail = v.Get("EMAIL");

			// we need the conneciton ID
			if(v.Find("CLIENTID") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_CLIENTID);
				return res;
			}

			// we shall be connected
			clientID = (int)v.Get("CLIENTID");
			if(!IsClientConnected(clientID, eMail))
			{
				res.Add("ERROR", PROTECT_NOT_CONNECTED);
				return res;
			}
			// all ok, return the key
			res.Add("KEY", appKey);
			break;
			
		case PROTECT_GETLICENSEINFO :

			// we need email
			if(v.Find("EMAIL") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_EMAIL);
				return res;
			}
			eMail = v.Get("EMAIL");

			// we need the conneciton ID
			if(v.Find("CLIENTID") < 0)
			{
				res.Add("ERROR", PROTECT_MISSING_CLIENTID);
				return res;
			}
			// we shall be connected
			clientID = (int)v.Get("CLIENTID");
			if(!IsClientConnected(clientID, eMail))
			{
				res.Add("ERROR", PROTECT_NOT_CONNECTED);
				return res;
			}
			// get user data from database
			res = db.Get(eMail);
			// remove unnecessary data
			res.RemoveKey("ACTIVATIONKEY");
			break;
			
		case PROTECT_UPDATEUSERDATA :
			{
				// we need email
				if(v.Find("EMAIL") < 0)
				{
					res.Add("ERROR", PROTECT_MISSING_EMAIL);
					return res;
				}
				eMail = v.Get("EMAIL");

				// we need the conneciton ID
				if(v.Find("CLIENTID") < 0)
				{
					res.Add("ERROR", PROTECT_MISSING_CLIENTID);
					return res;
				}
				
				// we shall be connected
				clientID = (int)v.Get("CLIENTID");
				if(!IsClientConnected(clientID, eMail))
				{
					res.Add("ERROR", PROTECT_NOT_CONNECTED);
					return res;
				}
				// get user data from database
				userRec = db.Get(eMail);
			
				// avoid hacking of source packet stripping
				// data non-updateable by user
				VectorMap<String, Value>vs(v, 1);
				vs.RemoveKey("LICENSES");
				vs.RemoveKey("EXPIRATION");
				vs.RemoveKey("ACTIVATIONKEY");
				vs.RemoveKey("EMAIL");
				
				// update user record
				for(int i = 0; i < vs.GetCount(); i++)
				{
					int j = userRec.Find(vs.GetKey(i));
					if(j >= 0)
						userRec[j] = vs[i];
				}
				db.Set(userRec);
			}
			break;

		default:
			// disconnect me anyways
			if(v.Find("CLIENTID") >= 0)
			{
				clientID = (int)v.Get("CLIENTID");
				DisconnectClient(clientID);
			}
			res.Add("ERROR", PROTECT_UNKNOWN_REASON);
			break;
	}
	return res;
}

// sends activation mail to user
bool ProtectServer::SendActivationMail(VectorMap<String, Value> const &userData)
{
	String body = welcomeBody;
	String subject = welcomeSubject;
	String key = userData.Get("ACTIVATIONKEY");
	
	// if no activation key field inside body, add it at end
	// trying to guess if it's HTML or simple text...
	int i;
	if( (i = body.Find("%ACTIVATIONKEY%")) < 0)
	{
		if(body.Find("HTML") >= 0)
			body << "&&&" << t_("ACTIVATION KEY : ") << key;
		else
			body << "\n\n\n" << t_("ACTIVATION KEY : ") << key;
	}
	else
		body.Replace(String("%ACTIVATIONKEY%"), key);
	body.Replace("%USER%", userData.Get("NAME"));
	subject.Replace("%USER%", userData.Get("NAME"));

	smtp.To(userData.Get("EMAIL"));
	smtp.Subject(subject);
	smtp.From(serverVars.Get("SERVER_NAME"));
	smtp.Text(body);
	return smtp.Send();
}

// sets client ids timeout value
ProtectServer &ProtectServer::SetIDCleanupTime(int t)
{
	idCleanupTime = t;
	lastIDCleanupTime = GetSysTime();
	return *this;
}

// sets database cleanup time
// used to clean up registrations with no activation on database
ProtectServer &ProtectServer::SetDBCleanupTime(int t)
{
	dbCleanupTime = t;
	lastDBCleanupTime = GetSysTime();
	return *this;
}

// runs the server
void ProtectServer::Run(void)
{
	// try to connect to db
	if(!db.Connect())
	{
		Cout() << "Error connecting to database\n";
		return;
	}
	
	// runs the SCGI server
	ScgiServer::Run();
}

END_UPP_NAMESPACE
