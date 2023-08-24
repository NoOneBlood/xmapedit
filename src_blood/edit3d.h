extern int InsertGameObject( int where, int nSector, int x, int y, int z, int nAngle);

void SetWave( int nSector, int nWave );
void SetSectorShadePhase( int nSector, int shadePhase );
void SetSectorTheta( int nSector, int bobTheta );

void SetFloorZ( int nSector, int z );
void SetCeilingZ( int nSector, int z );
void LowerFloor( int nSector, int nStep );
void LowerCeiling( int nSector, int nStep );
void RaiseFloor( int nSector, int nStep );
void RaiseCeiling( int nSector, int nStep );
void SetFloorRelative( int nSector, int dz );
void SetCeilingRelative( int nSector, int dz );
void SetFloorSlope( int nSector, int nSlope );
void SetCeilingSlope( int nSector, int nSlope );

void LowerSprite( spritetype *pSprite, int nStep );
void RaiseSprite( spritetype *pSprite, int nStep );
void PutSpriteOnFloor( spritetype *pSprite, int );
void PutSpriteOnCeiling( spritetype *pSprite, int );


void ProcessKeys3D();
void processMouseLook3D(BOOL readMouse = FALSE);

void AutoAlignWalls(int nWall0, int ply = 0);
char dlgSpriteText();