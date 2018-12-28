#include "AudioTabController.h"
#include <QQuickWindow>
#include <QApplication>
#include "../overlaycontroller.h"
#ifdef _WIN32
#    include "audiomanager/AudioManagerWindows.h"
#else
#    include "audiomanager/AudioManagerDummy.h"
#endif

// application namespace
<<<<<<< HEAD
namespace advsettings {

	void AudioTabController::initStage1() {
		vr::EVRSettingsError vrSettingsError;
        #ifdef _WIN32
		audioManager.reset(new AudioManagerWindows());
        #else
        audioManager.reset(new AudioManagerDummy());
        #endif
		audioManager->init(this);
		m_playbackDevices = audioManager->getPlaybackDevices();
		m_recordingDevices = audioManager->getRecordingDevices();
		findPlaybackDeviceIndex(audioManager->getPlaybackDevId(), false);
		char deviceId[1024];
		vr::VRSettings()->GetString(vr::k_pch_audio_Section, vr::k_pch_audio_OnPlaybackMirrorDevice_String, deviceId, 1024, &vrSettingsError);
		if (vrSettingsError != vr::VRSettingsError_None) {
			LOG(WARNING) << "Could not read \"" << vr::k_pch_audio_OnPlaybackMirrorDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);
		} else {
			audioManager->setMirrorDevice(deviceId);
			findMirrorDeviceIndex(audioManager->getMirrorDevId(), false);
			lastMirrorDevId = deviceId;
			m_mirrorVolume = audioManager->getMirrorVolume();
			m_mirrorMuted = audioManager->getMirrorMuted();
		}
		findMicDeviceIndex(audioManager->getMicDevId(), false);
		m_micVolume = audioManager->getMicVolume();
		m_micMuted = audioManager->getMicMuted();
		reloadPttProfiles();
		reloadPttConfig();
		reloadAudioProfiles();
		applyDefaultProfile();
		reloadAudioSettings();


		eventLoopTick();
	}


        void AudioTabController::initStage2(OverlayController * var_parent, QQuickWindow * var_widget) {
                this->parent = var_parent;
                this->widget = var_widget;

		std::string notifKey = std::string(OverlayController::applicationKey) + ".pptnotification";

		vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay(notifKey.c_str(), notifKey.c_str(), &m_ulNotificationOverlayHandle);
		if (overlayError == vr::VROverlayError_None) {
			std::string notifIconPath = QApplication::applicationDirPath().toStdString() + "/res/qml/ptt_notification.png";
			if (QFile::exists(QString::fromStdString(notifIconPath))) {
				vr::VROverlay()->SetOverlayFromFile(m_ulNotificationOverlayHandle, notifIconPath.c_str());
                                vr::VROverlay()->SetOverlayWidthInMeters(m_ulNotificationOverlayHandle, 0.02f);
                        vr::HmdMatrix34_t notificationTransform = {{
                                        {1.0f, 0.0f, 0.0f, 0.12f},
                                        {0.0f, 1.0f, 0.0f, 0.08f},
                                        {0.0f, 0.0f, 1.0f, -0.3f}
                                }};
				vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulNotificationOverlayHandle, vr::k_unTrackedDeviceIndex_Hmd, &notificationTransform);
			} else {
				LOG(ERROR) << "Could not find notification icon \"" << notifIconPath << "\"";
			}
		} else {
			LOG(ERROR) << "Could not create ptt notification overlay: " << vr::VROverlay()->GetOverlayErrorNameFromEnum(overlayError);
		}
	}

	void AudioTabController::reloadAudioSettings() {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		auto settings = OverlayController::appSettings();
		settings->beginGroup(getSettingsName());
		setMicProximitySensorCanMute(settings->value("micProximitySensorCanMute", false).toBool(), false);
		setMicReversePtt(settings->value("micReversePtt", false).toBool(), false);
		settings->endGroup();
	}

	void AudioTabController::saveAudioSettings() {
		auto settings = OverlayController::appSettings();
		settings->beginGroup(getSettingsName());
		settings->setValue("micProximitySensorCanMute", micProximitySensorCanMute());
		settings->setValue("micReversePtt", micReversePtt());
		settings->endGroup();
		settings->sync();
	}


	float AudioTabController::mirrorVolume() const {
		return m_mirrorVolume;
	}


	bool AudioTabController::mirrorMuted() const {
		return m_mirrorMuted;
	}


	float AudioTabController::micVolume() const {
		return m_micVolume;
	}


	bool AudioTabController::micMuted() const {
		return m_micMuted;
	}

	bool AudioTabController::micProximitySensorCanMute() const {
		return m_micProximitySensorCanMute;
	}

	bool AudioTabController::micReversePtt() const {
		return m_micReversePtt;
	}

	bool AudioTabController::audioProfileDefault() const {
		return m_isDefaultAudioProfile;
	}

	void AudioTabController::eventLoopTick() {
		if (!eventLoopMutex.try_lock()) {
			return;
		}
		if (settingsUpdateCounter >= 50) {
			if (m_micProximitySensorCanMute) {
				vr::VRControllerState_t controllerState;
				if (vr::VRSystem()->GetControllerState(vr::k_unTrackedDeviceIndex_Hmd, &controllerState, sizeof(vr::VRControllerState_t))) {
					if ((controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ProximitySensor)) == 0) {
						if (!m_micMuted) {
							setMicMuted(true);
						}
					} else {
						if (m_micMuted) {
							setMicMuted(false);
						}
					}
				} else {
					if (!parent->isDashboardVisible()) {
						LOG(ERROR) << "Could not read proximity sensor!";
					}
				}
			}
			vr::EVRSettingsError vrSettingsError;
			char mirrorDeviceId[1024];
			vr::VRSettings()->GetString(vr::k_pch_audio_Section, vr::k_pch_audio_OnPlaybackMirrorDevice_String, mirrorDeviceId, 1024, &vrSettingsError);
			if (vrSettingsError != vr::VRSettingsError_None) {
				LOG(WARNING) << "Could not read \"" << vr::k_pch_audio_OnPlaybackMirrorDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);
			}
			if (lastMirrorDevId.compare(mirrorDeviceId) != 0) {
				audioManager->setMirrorDevice(mirrorDeviceId);
				findMirrorDeviceIndex(audioManager->getMirrorDevId());
				lastMirrorDevId = mirrorDeviceId;
			}
			if (m_mirrorDeviceIndex >= 0) {
				setMirrorVolume(audioManager->getMirrorVolume());
				setMirrorMuted(audioManager->getMirrorMuted());
			}
			if (m_recordingDeviceIndex >= 0) {
				setMicVolume(audioManager->getMicVolume());
				setMicMuted(audioManager->getMicMuted());
			}
			settingsUpdateCounter = 0;
		} else {
			settingsUpdateCounter++;
		}
		checkPttStatus();
		eventLoopMutex.unlock();
	}

	bool AudioTabController::pttChangeValid() {
		return audioManager && audioManager->isMicValid();
	}

	void AudioTabController::onPttStart() {
		if (m_micReversePtt) {
			setMicMuted(true);
		} else {
			setMicMuted(false);
		}
	}

	void AudioTabController::onPttEnabled() {
		if (m_micReversePtt) {
			setMicMuted(false);
		} else {
			setMicMuted(true);
		}
	}

	void AudioTabController::onPttStop() {
		if (m_micReversePtt) {
			setMicMuted(false);
		} else {
			setMicMuted(true);
		}
	}

	void AudioTabController::onPttDisabled() {
		setMicMuted(false);
	}

	void AudioTabController::setMirrorVolume(float value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_mirrorVolume) {
			m_mirrorVolume = value;
			if (audioManager->isMirrorValid()) {
				audioManager->setMirrorVolume(value);
			}
			if (notify) {
				emit mirrorVolumeChanged(value);
			}
		}
	}

	void AudioTabController::setMirrorMuted(bool value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_mirrorMuted) {
			m_mirrorMuted = value;
			if (audioManager->isMirrorValid()) {
				audioManager->setMirrorMuted(value);
			}
			if (notify) {
				emit mirrorMutedChanged(value);
			}
		}
	}

	void AudioTabController::setMicVolume(float value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_micVolume) {
			m_micVolume = value;
			if (audioManager->isMicValid()) {
				audioManager->setMicVolume(value);
			}
			if (notify) {
				emit micVolumeChanged(value);
			}
		}
	}

	void AudioTabController::setMicMuted(bool value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_micMuted) {
			m_micMuted = value;
			if (audioManager->isMicValid()) {
				audioManager->setMicMuted(value);
			}
			if (notify) {
				emit micMutedChanged(value);
			}
		}
	}

	void AudioTabController::setMicProximitySensorCanMute(bool value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_micProximitySensorCanMute) {
			m_micProximitySensorCanMute = value;
			saveAudioSettings();
			if (notify) {
				emit micProximitySensorCanMuteChanged(value);
			}
		}
	}

	void AudioTabController::setMicReversePtt(bool value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_micReversePtt) {
			m_micReversePtt = value;
			saveAudioSettings();
			if (m_pttEnabled) {
				if (m_pttActive) {
					setMicMuted(value);
				} else {
					setMicMuted(!value);
				}
			}
			if (notify) {
				emit micReversePttChanged(value);
			}
		}
	}

	void AudioTabController::setAudioProfileDefault(bool value, bool notify) {
		std::lock_guard<std::recursive_mutex> lock(eventLoopMutex);
		if (value != m_isDefaultAudioProfile) {
			m_isDefaultAudioProfile = value;
			//TODO ????
			// ?? saveAudioSettings();
			if (notify) {
				emit audioProfileDefaultChanged(value);
			}
		}
	}

	void AudioTabController::onNewRecordingDevice() {
		findMicDeviceIndex(audioManager->getMicDevId());
	}

	void AudioTabController::onNewPlaybackDevice() {
		findPlaybackDeviceIndex(audioManager->getPlaybackDevId());
	}

	void AudioTabController::onNewMirrorDevice() {
		auto devid = audioManager->getMirrorDevId();
		if (devid.empty()) {
			m_mirrorDeviceIndex = -1;
			emit mirrorDeviceIndexChanged(m_mirrorDeviceIndex);
		} else {
			findMirrorDeviceIndex(devid);
		}
	}

	void AudioTabController::onDeviceStateChanged() {
		// I'm too lazy to find out which device has changed, so let's invalidate all device lists
		m_playbackDevices = audioManager->getPlaybackDevices();
		m_recordingDevices = audioManager->getRecordingDevices();
		findPlaybackDeviceIndex(audioManager->getPlaybackDevId(), false);
		findMirrorDeviceIndex(audioManager->getMirrorDevId(), false);
		findMicDeviceIndex(audioManager->getMicDevId(), false);
		emit playbackDeviceListChanged();
		emit recordingDeviceListChanged();
	}

        int AudioTabController::getPlaybackDeviceCount() {
            return static_cast<int>(m_playbackDevices.size());
	}

        QString AudioTabController::getPlaybackDeviceName(int index) {
            if (index > 0 && static_cast<size_t>(index) < m_playbackDevices.size()) {
                return QString::fromStdString(m_playbackDevices[static_cast<size_t>(index)].second);
		} else {
			return "<ERROR>";
		}
	}

        int AudioTabController::getRecordingDeviceCount() {
            return static_cast<int>(m_recordingDevices.size());
	}

	QString AudioTabController::getRecordingDeviceName(int index) {
                if (index > 0 && static_cast<size_t>(index) < m_playbackDevices.size()) {
                    return QString::fromStdString(m_recordingDevices[static_cast<size_t>(index)].second);
		} else {
			return "<ERROR>";
		}
	}


	int AudioTabController::playbackDeviceIndex() const {
		return m_playbackDeviceIndex;
	}

	int AudioTabController::mirrorDeviceIndex() const {
		return m_mirrorDeviceIndex;
	}

	int AudioTabController::micDeviceIndex() const {
		return m_recordingDeviceIndex;
	}

	void AudioTabController::setPlaybackDeviceIndex(int index, bool notify) {
		if (index != m_playbackDeviceIndex) {
                        if (index > 0 && static_cast<size_t>(index) < m_playbackDevices.size() && index != m_mirrorDeviceIndex) {
				vr::EVRSettingsError vrSettingsError;
				//Applys Audio Switch IN VR
                                vr::VRSettings()->SetString(vr::k_pch_audio_Section,
                                                            vr::k_pch_audio_OnPlaybackDevice_String,
                                                            m_playbackDevices[static_cast<size_t>(index)].first.c_str(),
                                                            &vrSettingsError);
				if (vrSettingsError != vr::VRSettingsError_None) {
					LOG(WARNING) << "Could not write \"" << vr::k_pch_audio_OnPlaybackDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);

				}
				else {
					vr::VRSettings()->Sync();
					audioManager->setPlaybackDevice(m_playbackDevices[index].first, notify);
				}
			}
			else if (notify) {
				emit playbackDeviceIndexChanged(m_playbackDeviceIndex);
			}
		}
	}

	void AudioTabController::setMirrorDeviceIndex(int index, bool notify) {
		if (index != m_mirrorDeviceIndex) {
			if (index == -1) {
				vr::EVRSettingsError vrSettingsError;
				vr::VRSettings()->RemoveKeyInSection(vr::k_pch_audio_Section, vr::k_pch_audio_OnPlaybackMirrorDevice_String, &vrSettingsError);
				if (vrSettingsError != vr::VRSettingsError_None) {
					LOG(WARNING) << "Could not remove \"" << vr::k_pch_audio_OnPlaybackMirrorDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);
				} else {
					vr::VRSettings()->Sync();
					audioManager->setMirrorDevice("", notify);
				}
                        } else if (index > 0 && static_cast<size_t>(index) < m_playbackDevices.size() && index != m_playbackDeviceIndex && index != m_mirrorDeviceIndex) {
                                vr::EVRSettingsError vrSettingsError;
                                vr::VRSettings()->SetString(vr::k_pch_audio_Section,
                                                            vr::k_pch_audio_OnPlaybackMirrorDevice_String,
                                                            m_playbackDevices[static_cast<size_t>(index)].first.c_str(),
                                                            &vrSettingsError);
				if (vrSettingsError != vr::VRSettingsError_None) {
					LOG(WARNING) << "Could not write \"" << vr::k_pch_audio_OnPlaybackMirrorDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);
				} else {
					vr::VRSettings()->Sync();
                                        audioManager->setMirrorDevice(m_playbackDevices[static_cast<size_t>(index)].first, notify);
				}
			} else if (notify) {
				emit mirrorDeviceIndexChanged(m_mirrorDeviceIndex);
			}
		}
	}

	void AudioTabController::setMicDeviceIndex(int index, bool notify) {
		if (index != m_recordingDeviceIndex) {
                        if (index > 0 && static_cast<size_t>(index) < m_recordingDevices.size()) {
                                vr::EVRSettingsError vrSettingsError;
                                vr::VRSettings()->SetString(vr::k_pch_audio_Section, vr::k_pch_audio_OnRecordDevice_String, m_recordingDevices[static_cast<size_t>(index)].first.c_str(), &vrSettingsError);
				if (vrSettingsError != vr::VRSettingsError_None) {
					LOG(WARNING) << "Could not write \"" << vr::k_pch_audio_OnRecordDevice_String << "\" setting: " << vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);
				} else {
                                        vr::VRSettings()->Sync();
                                        audioManager->setMicDevice(m_recordingDevices[static_cast<size_t>(index)].first, notify);
				}
			} else if (notify) {
				emit micDeviceIndexChanged(m_recordingDeviceIndex);
			}
		}
	}

	void AudioTabController::findPlaybackDeviceIndex(std::string id, bool notify) {
		int i = 0;
		bool deviceFound = false;
		for (auto d : m_playbackDevices) {
			if (d.first.compare(id) == 0) {
				deviceFound = true;
				break;
			} else {
				++i;
			}
		}
		if (deviceFound) {
			m_playbackDeviceIndex = i;
			if (notify) {
				emit playbackDeviceIndexChanged(i);
			}
		}
	}

	void AudioTabController::findMirrorDeviceIndex(std::string id, bool notify) {
		int i = 0;
		bool deviceFound = false;
		for (auto d : m_playbackDevices) {
			if (d.first.compare(id) == 0) {
				deviceFound = true;
				break;
			} else {
				++i;
			}
		}
		if (deviceFound && m_mirrorDeviceIndex != i) {
			m_mirrorDeviceIndex = i;
			if (notify) {
				emit mirrorDeviceIndexChanged(i);
			}
		}
	}

	void AudioTabController::findMicDeviceIndex(std::string id, bool notify) {
		int i = 0;
		bool deviceFound = false;
		for (auto d : m_recordingDevices) {
			if (d.first.compare(id) == 0) {
				deviceFound = true;
				break;
			} else {
				++i;
			}
		}
		if (deviceFound) {
			m_recordingDeviceIndex = i;
			if (notify) {
				emit micDeviceIndexChanged(i);
			}
		}
	}


/*AUDIO PROFILE FUNCTIONS
The following section includes the code required to save and load Audio
Profiles.

Saved Settings Include:
Playback Device
Mirror Device
Mirror Vol
Microphone
Microphone Volume
Profile Name

*/

/*
    Name:  reloadAudioProfiles

    Inputs: args: none
            other: Reads audioProfiles setting from file


    Output: return:none
            other:none

    Description: Clears working copy, and reloads from Settings file.

*/
void AudioTabController::reloadAudioProfiles()
{
    audioProfiles.clear();
    auto settings = OverlayController::appSettings();
    settings->beginGroup( getSettingsName() );
    auto profileCount = settings->beginReadArray( "audioProfiles" );
    for ( int i = 0; i < profileCount; i++ )
    {
        settings->setArrayIndex( i );
        audioProfiles.emplace_back();
        auto& entry = audioProfiles[static_cast<size_t>( i )];
        entry.profileName
            = settings->value( "profileName" ).toString().toStdString();
        entry.playbackName
            = settings->value( "playbackName" ).toString().toStdString();
        entry.micName = settings->value( "micName" ).toString().toStdString();
        entry.mirrorName
            = settings->value( "mirrorName" ).toString().toStdString();
        entry.micMute = settings->value( "micMute", false ).toBool();
        entry.mirrorMute = settings->value( "mirrorMute", false ).toBool();
        entry.mirrorVol = settings->value( "mirrorVol", 0.0 ).toFloat();
        entry.micVol = settings->value( "micVol", 1.0 ).toFloat();
    }
    settings->endArray();
    settings->endGroup();
}

/*AUDIO PROFILE FUNCTIONS
The following section includes the code required to save and load Audio
Profiles.

Saved Settings Include:
Playback Device
Mirror Device
Mirror Vol
Microphone
Microphone Volume
Profile Name

*/

/*
Name:  reloadAudioProfiles

Inputs: args: none
other: Reads audioProfiles setting from file


Output: return:none
other:none

Description: Clears working copy, and reloads from Settings file.

*/
void AudioTabController::reloadAudioProfiles()
{
	audioProfiles.clear();
	auto settings = OverlayController::appSettings();
	settings->beginGroup(getSettingsName());
	auto profileCount = settings->beginReadArray("audioProfiles");
	for (int i = 0; i < profileCount; i++)
	{
		settings->setArrayIndex(i);
		audioProfiles.emplace_back();
		auto& entry = audioProfiles[static_cast<size_t>(i)];
		entry.profileName
			= settings->value("profileName").toString().toStdString();
		entry.playbackName
			= settings->value("playbackName").toString().toStdString();
		entry.micName = settings->value("micName").toString().toStdString();
		entry.mirrorName
			= settings->value("mirrorName").toString().toStdString();
		entry.micMute = settings->value("micMute", false).toBool();
		entry.mirrorMute = settings->value("mirrorMute", false).toBool();
		entry.mirrorVol = settings->value("mirrorVol", 0.0).toFloat();
		entry.micVol = settings->value("micVol", 1.0).toFloat();
	}
	settings->endArray();
	settings->endGroup();
}

/*
Name: saveAudioProfile

Inputs: args: none
other: none


Output: return: none
other: Writes audioProfiles setting to file

Description: saves a copy of the audio profiles from working to Settings file.

*/
void AudioTabController::saveAudioProfiles()
{
	auto settings = OverlayController::appSettings();
	settings->beginGroup(getSettingsName());
	settings->beginWriteArray("audioProfiles");
	int i = 0;
	for (auto& p : audioProfiles)
	{
		settings->setArrayIndex(i);
		settings->setValue("profileName",
			QString::fromStdString(p.profileName));
		settings->setValue("playbackName",
			QString::fromStdString(p.playbackName));
		settings->setValue("micName", QString::fromStdString(p.micName));
		settings->setValue("mirrorName",
			QString::fromStdString(p.mirrorName));
		settings->setValue("micMute", p.micMute);
		settings->setValue("mirrorMute", p.mirrorMute);
		settings->setValue("micVol", p.micVol);
		settings->setValue("mirrorVol", p.mirrorVol);
		i++;
	}
	settings->endArray();
	settings->endGroup();
}

/*
Name: addAudioProfile

Inputs: args: Qstring name - the name of the audio profile
other: none


Output: return: none
other: audioProfile to audioProfiles array

Description: Creates an audioProfile, and adds it to the working copy of
audioProfiles[]

*/

void AudioTabController::addAudioProfile(QString name)
{
	AudioProfile* profile = nullptr;
	for (auto& p : audioProfiles)
	{
		if (p.profileName.compare(name.toStdString()) == 0)
		{
			profile = &p;
			break;
		}
	}
	if (!profile)
	{
		auto i = audioProfiles.size();
		audioProfiles.emplace_back();
		profile = &audioProfiles[i];
	}
	profile->profileName = name.toStdString();
	profile->playbackName
		= getPlaybackDeviceName(m_playbackDeviceIndex).toStdString();
	profile->mirrorName
		= getPlaybackDeviceName(m_mirrorDeviceIndex).toStdString();
	profile->micName
		= getRecordingDeviceName(m_recordingDeviceIndex).toStdString();
	profile->micMute = m_micMuted;
	profile->mirrorMute = m_mirrorMuted;
	profile->mirrorVol = m_mirrorVolume;
	profile->micVol = m_micVolume;

	saveAudioProfiles();
	OverlayController::appSettings()->sync();
	emit audioProfilesUpdated();
}

/*
Name: applyAudioProfile

Inputs: args: index - index of audioProfile in audioProfiles[]
other: audioProfile based on index


Output: return: none
other: none

Description: Applies the required logic to activate the audio profile.

*/
// TODO Remembers Mirror Volume when switching to main volume.
void AudioTabController::applyAudioProfile(unsigned index)
{
	if (index < audioProfiles.size())
	{
		auto& profile = audioProfiles[index];
		int mInd = getMirrorIndex(profile.mirrorName);
		int pInd = getPlaybackIndex(profile.playbackName);

		// Needed to keep remembering when swtiching from mirror/main etc.
		setMicMuted(false, false);
		setMirrorMuted(false, false);

		if ((m_playbackDeviceIndex == mInd)
			&& (m_mirrorDeviceIndex == pInd))
		{
			setMirrorDeviceIndex(-1, true);
		}
		if (m_playbackDeviceIndex == mInd)
		{
			setPlaybackDeviceIndex(pInd, true);
			setMirrorDeviceIndex(mInd, true);
		}
		else
		{
			setMirrorDeviceIndex(mInd, true);
			setPlaybackDeviceIndex(pInd, true);
		}
		setMicDeviceIndex(getRecordingIndex(profile.micName), true);
		setMicMuted(profile.micMute, true);
		setMirrorMuted(profile.mirrorMute, true);
		setMicVolume(profile.micVol, true);
		setMirrorVolume(profile.mirrorVol, true);
	}
}

/*
Name: deleteAudioProfile

Inputs: args: index - index of audioProfile in audioProfiles[]
other: audioProfile based on index


Output: return: none
other: none

Description: Removes selected Audio Profile

*/
void AudioTabController::deleteAudioProfile(unsigned index)
{
	if (index < audioProfiles.size())
	{
		auto pos = audioProfiles.begin() + index;
		audioProfiles.erase(pos);
		saveAudioProfiles();
		OverlayController::appSettings()->sync();
		emit audioProfilesUpdated();
	}
}

unsigned AudioTabController::getAudioProfileCount()
{
	return static_cast<unsigned int>(audioProfiles.size());
}

QString AudioTabController::getAudioProfileName(unsigned index)
{
	if (index < audioProfiles.size())
	{
		return QString::fromStdString(audioProfiles[index].profileName);
	}
	return "";
}

/*
Name: getPlaybackIndex,  getRecordingIndex, and getMirrorIndex

input: string, of microphone/playback device name
output: integer for use In: setMicDeviceIndex(int,bool),
setMirrorDeviceIndex(int,bool) setPlayBackDeviceIndex(int,bool)

description: Gets proper index value for selecting specific devices.
*/
int AudioTabController::getPlaybackIndex(std::string str)
{
	// Unsigned to avoid two casts. i won't be negative because we only
	// increment it.
	for (unsigned int i = 0; i < m_playbackDevices.size(); i++)
	{
		if (str.compare(m_playbackDevices[i].second) == 0)
		{
			return static_cast<int>(i);
		}
	}
	return m_playbackDeviceIndex;
}

int AudioTabController::getRecordingIndex(std::string str)
{
	// Unsigned to avoid two casts. i won't be negative because we only
	// increment it.
	for (unsigned int i = 0; i < m_recordingDevices.size(); i++)
	{
		if (str.compare(m_recordingDevices[i].second) == 0)
		{
			return static_cast<int>(i);
		}
	}
	return m_recordingDeviceIndex;
}

int AudioTabController::getMirrorIndex(std::string str)
{
	// Unsigned to avoid two casts. i won't be negative because we only
	// increment it.
	for (unsigned int i = 0; i < m_playbackDevices.size(); i++)
	{
		if (str.compare(m_playbackDevices[i].second) == 0)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}

/* ---------------------------*/

/*----------------*/

} // namespace advconfig
