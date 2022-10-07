function login()
{
   if ($Client::LoggedIn)
      return;
      
   execPrefs("account.cs");
   
   if ($Account::Token !$= "")
   {
      if(WebAPILoginSession($Account::Token))
         return;
   }
   
   showLogin();   
}

function showLogin()
{
   error("TODO: Show Login Form");
}

function onWebAPILoginComplete(%loggedIn, %statusMsg, %token, %displayName)
{
   echo("onWebAPILoginComplete: " @ %statusMsg);   
   
   if (!%loggedIn)
      return;
      
   $Player::Name = %displayName;
   
   $Account::DisplayName = %displayName;
   $Account::Token = %token;
   
   $Client::LoggedIn = true;
   
   echo("Logged in as: " @ $Account::DisplayName);
   
   savePCUserProfile();
}

function onWebAPILogoutComplete(%loggedOut, %statusMsg)
{
   echo("onWebAPILogoutComplete: " @ %statusMsg);   
   
   //if (!%loggedOut)
   //   return;
      
   $Player::Name = XBLiveGetUserName();
   
   $Account::DisplayName = "";
   $Account::Token = "";
   
   $Client::LoggedIn = false;
   
   echo("Logged out");
   
   savePCUserProfile();
}
