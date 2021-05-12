if (!isObject(MusicPlayer))
{
    new ScriptGroup(MusicPlayer)
    {
        playOnActivate = 0;
        isActive = 0;
        currTrack = 0;
    };
}

function MusicPlayer::activate(%this)
{
   %this.isActive = 1;
   
   if (!%this.playOnActivate || (isObject(%this.currTrack) && %this.currTrack.isPlaying()))
      return;
      
   %this.playNext();
}

function MusicPlayer::isPlaying(%this)
{
   return isObject(%this.currTrack) && %this.currTrack.isPlaying();
}

function MusicPlayer::stop(%this)
{
   if (isObject(%this.currTrack) && %this.currTrack.isPlaying())
   {
      %this.currTrack.delete();
   }
}

function MusicPlayer::addTrack(%this, %trackSfxProfile)
{
   if (!%this.isMember(%trackSfxProfile))
   {
      %this.add(%trackSfxProfile);
      if (%this.playOnActivate && %this.isActive && !%this.isPlaying())
         %this.playNext();
   }
}

function MusicPlayer::play(%this)
{
   if (%this.isPlaying())
      return;
   
   %this.playNext();
}

function MusicPlayer::playNext(%this)
{
   %this.stop();
   
   if (%this.currTrackIndex $= "")
      %this.currTrackIndex = 0;
   else
      %this.currTrackIndex = %this.currTrackIndex + 1;
   
   if (%this.currTrackIndex >= %this.getCount())
      %this.currTrackIndex = 0;
   if (%this.currTrackIndex < %this.getCount())
      %this.currTrack = sfxPlay(%this.getObject(%this.currTrackIndex));
}
