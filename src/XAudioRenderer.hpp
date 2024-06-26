#ifndef X_AUDIO_RENDERER_HPP
#define X_AUDIO_RENDERER_HPP

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "Ole32.lib")

#include <windows.h>
#include <xaudio2.h>

class XAudioRenderer
{
public:
    static const int numSourceVoices = 32;
    static const int numLoadableSounds = 16;
    IXAudio2 *pXAudio2 = nullptr;
    IXAudio2SourceVoice *pSourceVoice[numSourceVoices];
    WAVEFORMATEXTENSIBLE wfx = {0};
    XAUDIO2_BUFFER buffers[numLoadableSounds] = {0};
    unsigned int loadedSounds = 0;

    XAudioRenderer()
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

        IXAudio2MasteringVoice *pMasterVoice = nullptr;
        hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);

        hr = LoadSound(__TEXT("assets/bloop.wav"));
        hr = LoadSound(__TEXT("assets/explode.wav"));
        hr = LoadSound(__TEXT("assets/blung.wav"));

        // start xaudio
        for (int i = 0; i < numSourceVoices; ++i)
        {
            hr = pXAudio2->CreateSourceVoice(&pSourceVoice[i], (WAVEFORMATEX *)&wfx);
        }
    }

    void Play(unsigned int soundIndex=0, float pitchChange = 1.0f)
    {
        for (int i = 0; i < numSourceVoices; ++i)
        {
            XAUDIO2_VOICE_STATE voiceState;
            pSourceVoice[i]->GetState(&voiceState);

            if (voiceState.BuffersQueued <= 0)
            {
                // Sound has finished playing, start a new one
                pSourceVoice[i]->Stop(0);
                pSourceVoice[i]->SetFrequencyRatio(pitchChange);
                pSourceVoice[i]->FlushSourceBuffers();
                pSourceVoice[i]->SubmitSourceBuffer(&(buffers[soundIndex]));
                pSourceVoice[i]->Start(0);
                return;
            }
        }
    }

private:
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

    HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD &dwChunkSize, DWORD &dwChunkDataPosition)
    {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD dwChunkType;
        DWORD dwChunkDataSize;
        DWORD dwRIFFDataSize = 0;
        DWORD dwFileType;
        DWORD bytesRead = 0;
        DWORD dwOffset = 0;

        while (hr == S_OK)
        {
            DWORD dwRead;
            if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            switch (dwChunkType)
            {
            case fourccRIFF:
                dwRIFFDataSize = dwChunkDataSize;
                dwChunkDataSize = 4;
                if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                    hr = HRESULT_FROM_WIN32(GetLastError());
                break;

            default:
                if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                    return HRESULT_FROM_WIN32(GetLastError());
            }

            dwOffset += sizeof(DWORD) * 2;

            if (dwChunkType == fourcc)
            {
                dwChunkSize = dwChunkDataSize;
                dwChunkDataPosition = dwOffset;
                return S_OK;
            }

            dwOffset += dwChunkDataSize;

            if (bytesRead >= dwRIFFDataSize)
                return S_FALSE;
        }

        return S_OK;
    }

    HRESULT ReadChunkData(HANDLE hFile, void *buffer, DWORD buffersize, DWORD bufferoffset)
    {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());
        DWORD dwRead;
        if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    HRESULT LoadSound(TCHAR* soundPath)
    {
        // load sound

        TCHAR *strFileName = soundPath;
        // Open the file
        HANDLE hFile = CreateFile(
            strFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (INVALID_HANDLE_VALUE == hFile)
            return HRESULT_FROM_WIN32(GetLastError());

        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD dwChunkSize;
        DWORD dwChunkPosition;
        // check the file type, should be fourccWAVE or 'XWMA'
        FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
        DWORD filetype;
        ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
        if (filetype != fourccWAVE)
            return S_FALSE;

        FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
        ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

        // fill out the audio data buffer with the contents of the fourccDATA chunk
        FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
        BYTE *pDataBuffer = new BYTE[dwChunkSize];
        ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

        buffers[loadedSounds].AudioBytes = dwChunkSize;      // size of the audio buffer in bytes
        buffers[loadedSounds].pAudioData = pDataBuffer;      // buffer containing audio data
        buffers[loadedSounds].Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

        loadedSounds++;
        return S_OK;
    }
};

#endif