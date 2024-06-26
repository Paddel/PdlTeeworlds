
#ifndef ENGINE_CLIENT_CLIENT_H
#define ENGINE_CLIENT_CLIENT_H

#include <base/tl/sorted_array.h>

struct CMapDownloadChunk
{
	unsigned char m_aData[900];
	int m_Size;
	int m_Chunk;

	CMapDownloadChunk(int Chunk, unsigned char *pData, int Size)
	{
		m_Chunk = Chunk;
		mem_copy(m_aData, pData, Size);
		m_Size = Size;
	}
	CMapDownloadChunk() {}

	bool operator<(const CMapDownloadChunk &Other) { return m_Chunk < Other.m_Chunk; }
};

class CGraph
{
public:
	enum
	{
		// restrictions: Must be power of two
		MAX_VALUES=128,
	};

	float m_Min, m_Max;
	float m_aValues[MAX_VALUES];
	float m_aColors[MAX_VALUES][3];
	int m_Index;

	void Init(float Min, float Max);

	void ScaleMax();
	void ScaleMin();

	void Add(float v, float r, float g, float b);
	void Render(IGraphics *pGraphics, int Font, float x, float y, float w, float h, const char *pDescription);
};


class CSmoothTime
{
	int64 m_Snap;
	int64 m_Current;
	int64 m_Target;

	int64 m_RLast;
	int64 m_TLast;
	CGraph m_Graph;

	int m_SpikeCounter;

	float m_aAdjustSpeed[2]; // 0 = down, 1 = up
public:
	void Init(int64 Target);
	void SetAdjustSpeed(int Direction, float Value);

	int64 Get(int64 Now);

	void UpdateInt(int64 Target);
	void Update(CGraph *pGraph, int64 Target, int TimeLeft, int AdjustDirection);
};


class CClient : public IClient, public CDemoPlayer::IListner
{
	// needed interfaces
	IEngine *m_pEngine;
	IEditor *m_pEditor;
	IEngineInput *m_pInput;
	IEngineGraphics *m_pGraphics;
	IEngineSound *m_pSound;
	IGameClient *m_pGameClient;
	IEngineMap *m_pMap;
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	IEngineMasterServer *m_pMasterServer;

	enum
	{
		NUM_SNAPSHOT_TYPES=2,
		PREDICTION_MARGIN=1000/50/2, // magic network prediction value
	};

	class CNetClient m_NetClient;
	class CDemoPlayer m_DemoPlayer;
	class CDemoRecorder m_DemoRecorder;
	class CServerBrowser m_ServerBrowser;
	class CFriends m_Friends;
	class CMapChecker m_MapChecker;

	char m_aServerAddressStr[256];

	unsigned m_SnapshotParts;
	int64 m_LocalStartTime;

	int m_DebugFont;
	
	int64 m_LastRenderTime;
	float m_RenderFrameTimeLow;
	float m_RenderFrameTimeHigh;
	int m_RenderFrames;

	NETADDR m_ServerAddress;
	int m_WindowMustRefocus;
	int m_SnapCrcErrors;
	bool m_AutoScreenshotRecycle;
	bool m_EditorActive;
	bool m_SoundInitFailed;
	bool m_ResortServerBrowser;

	int m_AckGameTick;
	int m_CurrentRecvTick;
	int m_RconAuthed;
	int m_UseTempRconCommands;

	// version-checking
	char m_aVersionStr[10];

	// pinging
	int64 m_PingStartTime;

	//
	char m_aCurrentMap[256];
	unsigned m_CurrentMapCrc;

	//
	char m_aCmdConnect[256];

	// map download
	char m_aMapdownloadFilename[256];
	char m_aMapdownloadName[256];
	IOHANDLE m_MapdownloadFile;
	int m_MapdownloadChunk;
	int m_MapdownloadCrc;
	int m_MapdownloadAmount;
	int m_MapdownloadTotalsize;
	bool m_MapdownloadQuick;
	bool m_MapdlownloadNextType;
	int m_MapdownloadMaxChunk;
	sorted_array<CMapDownloadChunk> m_MapdownloadChunks;

	void NewMapChunk(int Last, int MapCRC, int Chunk, int Size, const unsigned char *pData, bool RequestNext);
	bool MapdownloadChunkDownloaded(int Chunk);
	int MapdownloadFindWantedChunk();

	// time
	CSmoothTime m_GameTime;
	CSmoothTime m_PredictedTime;

	// input
	struct // TODO: handle input better
	{
		int m_aData[MAX_INPUT_SIZE]; // the input data
		int m_Tick; // the tick that the input is for
		int64 m_PredictedTime; // prediction latency when we sent this input
		int64 m_Time;
	} m_aInputs[200];

	int m_CurrentInput;

	// graphs
	CGraph m_InputtimeMarginGraph;
	CGraph m_GametimeMarginGraph;
	CGraph m_FpsGraph;

	// the game snapshots are modifiable by the game
	class CSnapshotStorage m_SnapshotStorage;
	CSnapshotStorage::CHolder *m_aSnapshots[NUM_SNAPSHOT_TYPES];

	int m_RecivedSnapshots;
	char m_aSnapshotIncommingData[CSnapshot::MAX_SIZE];

	class CSnapshotStorage::CHolder m_aDemorecSnapshotHolders[NUM_SNAPSHOT_TYPES];
	char *m_aDemorecSnapshotData[NUM_SNAPSHOT_TYPES][2][CSnapshot::MAX_SIZE];

	class CSnapshotDelta m_SnapshotDelta;

	//
	class CServerInfo m_CurrentServerInfo;
	int64 m_CurrentServerInfoRequestTime; // >= 0 should request, == -1 got info

	// version info
	struct CVersionInfo
	{
		enum
		{
			STATE_INIT=0,
			STATE_START,
			STATE_READY,
		};

		int m_State;
		class CHostLookup m_VersionServeraddr;
	} m_VersionInfo;

	int m_ReinitWindowCount;

	volatile int m_GfxState;
	static void GraphicsThreadProxy(void *pThis) { ((CClient*)pThis)->GraphicsThread(); }
	void GraphicsThread();

public:
	IEngine *Engine() { return m_pEngine; }
	IEngineGraphics *Graphics() { return m_pGraphics; }
	IEngineInput *Input() { return m_pInput; }
	IEngineSound *Sound() { return m_pSound; }
	IGameClient *GameClient() { return m_pGameClient; }
	IEngineMasterServer *MasterServer() { return m_pMasterServer; }
	IStorage *Storage() { return m_pStorage; }

	CClient();

	// ----- send functions -----
	virtual int SendMsg(CMsgPacker *pMsg, int Flags, bool System = false);

	int SendMsgEx(CMsgPacker *pMsg, int Flags, bool System=true);
	void SendInfo();
	void SendEnterGame();
	void SendReady();

	virtual int SendMsgDummy(CMsgPacker *pMsg, int Flags, int Index, bool System=true);

	virtual bool RconAuthed() { return m_RconAuthed != 0; }
	virtual bool UseTempRconCommands() { return m_UseTempRconCommands != 0; }
	void RconAuth(const char *pName, const char *pPassword);
	virtual void Rcon(const char *pCmd);

	virtual bool ConnectionProblems();

	virtual bool SoundInitFailed() { return m_SoundInitFailed; }

	virtual int GetDebugFont() { return m_DebugFont; }

	void DirectInput(int *pInput, int Size);
	void SendInput();

	virtual char *GetServerAddress() { return m_aServerAddressStr; }

	// TODO: OPT: do this alot smarter!
	virtual int *GetInput(int Tick);

	const char *LatestVersion();
	void VersionUpdate();

	// ------ state handling -----
	void SetState(int s);

	// called when the map is loaded and we should init for a new round
	void OnEnterGame();
	virtual void EnterGame();

	virtual void Connect(const char *pAddress);
	void DisconnectWithReason(const char *pReason);
	virtual void Disconnect();

	struct
	{
		class CNetClient m_NetClient;
		bool m_Inited;
		bool m_Online;
		int64 m_LastConnect;

	} m_aDummy[MAX_DUMMIES];

	int m_DummyCamera;
	int m_DummyControl;

	int m_aInputList[10];

	virtual void DummyToggle(int Index);
	virtual void DummyConnect(int Index);
	virtual void DummyDisonnect(int Index);
	virtual void DummyOnMain(int Index);
	virtual int GetDummyCam() { return m_DummyCamera; }
	virtual int GetDummyActive(int Dummy) { return m_aDummy[Dummy].m_Online; }
	virtual int GetDummyControl() { return m_DummyControl; }
	void SaveDummyInfos();

	virtual void GetServerInfo(CServerInfo *pServerInfo);
	void ServerInfoRequest();

	bool m_Autojoin;
	NETADDR m_AutojoinAddress;
	int64 m_AutojoinInfoTime;

	virtual bool GetAutojoin() { return m_Autojoin; }
	virtual NETADDR *GetAutojoinAddr() { return &m_AutojoinAddress; }

	int LoadData();

	// ---

	void *SnapGetItem(int SnapID, int Index, CSnapItem *pItem);
	void SnapInvalidateItem(int SnapID, int Index);
	void *SnapFindItem(int SnapID, int Type, int ID);
	int SnapNumItems(int SnapID);
	void SnapSetStaticsize(int ItemType, int Size);

	void Render();
	void DebugRender();

	virtual void Quit();

	virtual bool IsDDRace();

	virtual const char *ErrorString();

	const char *LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc);
	const char *LoadMapSearch(const char *pMapName, int WantedCrc);

	static int PlayerScoreComp(const void *a, const void *b);

	void ProcessConnlessPacket(CNetChunk *pPacket);
	void ProcessServerPacket(CNetChunk *pPacket);
	void ProcessServerPacketDummy(CNetChunk *pPacket, int Index);

	virtual int MapDownloadAmount() { return m_MapdownloadAmount; };
	virtual int MapDownloadTotalsize() { return m_MapdownloadTotalsize; };
	virtual unsigned CurrentMapCrc() { return m_CurrentMapCrc; };
	virtual char *CurrentMapName() { return m_aCurrentMap; };

	virtual int ReinitWindowCount() { return m_ReinitWindowCount; }

	void PumpNetwork();

	virtual void OnDemoPlayerSnapshot(void *pData, int Size);
	virtual void OnDemoPlayerMessage(void *pData, int Size);

	virtual int AckGameTick() { return m_AckGameTick; }

	void Update();

	void RegisterInterfaces();
	void InitInterfaces();

	void Run();

	static void Con_PdlDummyToggle(IConsole::IResult *pResult, void *pUserData);
	static void Con_PdlDummyConnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_PdlDummyDisconnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_PdlDummyControl(IConsole::IResult *pResult, void *pUserData);
	static void Con_PdlDummyOnMain(IConsole::IResult *pResult, void *pUserData);
	static void Con_Connect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Disconnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Quit(IConsole::IResult *pResult, void *pUserData);
	static void Con_Minimize(IConsole::IResult *pResult, void *pUserData);
	static void Con_Ping(IConsole::IResult *pResult, void *pUserData);
	static void Con_Screenshot(IConsole::IResult *pResult, void *pUserData);
	static void Con_Rcon(IConsole::IResult *pResult, void *pUserData);
	static void Con_RconAuth(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_RemoveFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_Play(IConsole::IResult *pResult, void *pUserData);
	static void Con_Record(IConsole::IResult *pResult, void *pUserData);
	static void Con_StopRecord(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddDemoMarker(IConsole::IResult *pResult, void *pUserData);
	static void ConchainServerBrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void Con_HideConsole(IConsole::IResult *pResult, void *pUserData);

	void RegisterCommands();

	const char *DemoPlayer_Play(const char *pFilename, int StorageType);
	void DemoRecorder_Start(const char *pFilename, bool WithTimestamp);
	void DemoRecorder_HandleAutoStart();
	void DemoRecorder_Stop();
	void DemoRecorder_AddDemoMarker();

	void AutoScreenshot_Start();
	void AutoScreenshot_Cleanup();

	void ServerBrowserUpdate();

	void ReinitWindow();
	void FirstOpen();
};
#endif
