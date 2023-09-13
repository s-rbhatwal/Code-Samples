#include "glm/vec4.hpp"
#include "glm/vec3.hpp"

class BehaviorComponent;


class BehaviorPlayer : public BehaviorComponent
{
public:
	virtual void Init();
	virtual void Update(float dt);
	virtual ~BehaviorPlayer();
	void ResetJumpCounter() { CurrentJumpCount = 0; }
	void SetCurrentPlanetoid(Object* Planetoid) { PlayerPlanetoid = Planetoid; }
	void DoIceMovement(float dt);
	void JumpControl(float dt);
	void CameraControl(float dt);
	void StartSkate();
	void SpriteRotation();
	void ChangeUpDirection(glm::vec4* NewUp, float AngleOfRotation);
	Object* GetCurrentPlanetoid() { return PlayerPlanetoid; }
private:
	float TimeSinceLastJump = 0;
	glm::vec4 SkatingPlayerForwardDirection = glm::vec4(0, 0, 1, 0);
	glm::vec4 CurrentUpDirection = glm::vec4(0, 1, 0,0);

	int MaxJumpsAtATime = 1;
	int CurrentJumpCount = 0;
	float SkateSpeedForward = 0;
	Object* PlayerSpriteObject = 0;
	Object* PlayerPlanetoid = 0;
};

