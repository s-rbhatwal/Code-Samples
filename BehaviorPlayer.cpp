#include "BehaviorComponent.h"
#include "BehaviorPlayer.h"
#include "Transform.h"
#include "PhysicsComponent.h"
#include "Object.h"
#include "SuperRiti3DWorld.h"
#include "Scene.h"
#include "GLFW/glfw3.h"
#include "Camera.h"
#include "Affine.h"
#include "Trace.h"
#include "Material.h"
#include "Collider.h"
#include "CollisionManifest.h"

const float PI = 4.0f * atan(1.0f);

glm::vec4 RemoveComponentInDir(glm::vec4* RemoveDirectionNormed, glm::vec4* VectorToRemoveFrom)//helper function
{
	glm::vec4 PieceToRemove = glm::dot(*VectorToRemoveFrom, *RemoveDirectionNormed) * (*RemoveDirectionNormed);
	return (*VectorToRemoveFrom) - (PieceToRemove);
}

void BehaviorPlayer::Init()
{
	//this is the initialization of a BehaviorPlayer, which has the Player game object as it's parent
	PlayerSpriteObject = new Object("Cube", "None", "None", "None", false);//PlayerSpriteObject is another game object that is owned by the Player game object
	PlayerSpriteObject->GetTransform()->SetScale(1.0f, 1.0f, 0.05f);
	PlayerSpriteObject->GetTransform()->SetParentTransform(GetParent()->GetTransform());
	glm::vec3 OutLineColor(1, 1, 1);
	GetCurrentScene()->AddObject(PlayerSpriteObject);
	GetParent()->GetMaterial()->SetDebugDrawColor(&OutLineColor);
	GetParent()->GetTransform()->UseEulerAnglesFlag(false);//this means that the Player game object will not store x,y and z rotation values. The object's rotation transformation matrix will be directly modified instead.
}

void BehaviorPlayer::Update(float dt)
{
	if (PlayerPlanetoid != 0)
	{
		glm::vec4 GroundNormal = (*GetParent()->GetCollider()->GetLastMVTDirection());
		float AngleBetweenCurrentUpAndGroundNormal = acos(glm::dot(CurrentUpDirection, GroundNormal)) * 180 / PI;//this value is in degrees

		if (GetParent()->GetCollider()->GetLastCollidedWith() == PlayerPlanetoid && !isnan(AngleBetweenCurrentUpAndGroundNormal) && AngleBetweenCurrentUpAndGroundNormal > 1.0f)
		{
			ChangeUpDirection(&GroundNormal, AngleBetweenCurrentUpAndGroundNormal);
		}
	}

	glm::vec3 GravityAcceleration(CurrentUpDirection.x, CurrentUpDirection.y, CurrentUpDirection.z);
	GravityAcceleration *= -10.0f;
	GetParent()->GetPhysics()->SetAcceleration(&GravityAcceleration);

	DoIceMovement(dt);// the movement of the player ice skating on the planetoid
	SpriteRotation(); //the game object is a 3d cube, but there is a sprite that moves with it to display the player character
	JumpControl(dt);
	CameraControl(dt);
}

void BehaviorPlayer::JumpControl(float dt)
{
	if (GetSpacePressed() && TimeSinceLastJump > 0.25f && CurrentJumpCount < MaxJumpsAtATime)
	{
		float JumpPower = 8;
		CurrentJumpCount++;
		glm::vec3 JumpVel (0,0,0);
		if (!GetGravityOn())//jump behaves differently if no gravity
		{
			JumpVel = JumpPower * CurrentUpDirection;
		}
		else
		{
			JumpVel = JumpPower * CurrentUpDirection;
		}
		glm::vec3 NewVel = *(GetParent()->GetPhysics()->GetVelocity()) + JumpVel;
		GetParent()->GetPhysics()->SetVelocity(&NewVel);

		TimeSinceLastJump = 0;
	}
	TimeSinceLastJump += dt;
}


void BehaviorPlayer::DoIceMovement(float dt)
{
	glm::vec4 PlayerVel(*GetParent()->GetPhysics()->GetVelocity(), 0);
	// we dont want to cancel out movement in the direction of the current up vector, so retain it.
	float UpVel = glm::dot(PlayerVel, CurrentUpDirection);
	glm::vec4 VelPerpendicularToGround = UpVel * CurrentUpDirection;//<- it's retained here in VelPerpendicularToGround

	//however , we do want to cap "VelPerpendicularToGround" because the gravity's acceleration can make it grow to a large magnitude
	if (UpVel < -3)
	{
		VelPerpendicularToGround = (-3.0f * CurrentUpDirection);
	}
	
	glm::vec3 Input = GetInput();
	
	float TurnSpeed = 100;
	if (CurrentJumpCount > 0)//player has jumped and is in air
	{
		TurnSpeed *= 2;
	}
	
	glm::mat4x4 RotationMat(1.0);
	if (Input.x < 0)
	{
		RotationMat = Aff_Rotate(TurnSpeed * dt, CurrentUpDirection);
	}
	if (Input.x > 0)
	{
		RotationMat = Aff_Rotate(-TurnSpeed * dt, CurrentUpDirection);
	}
	SkatingPlayerForwardDirection = RotationMat  * SkatingPlayerForwardDirection;

	glm::mat4x4 NewRotation = RotationMat * GetParent()->GetTransform()->GetOrientationMatrix();
	GetParent()->GetTransform()->SetOrientationMatrix(NewRotation);

	float SkateAcceleration = 10.0f;
	float SkateDecceleration = 1.0f;
	float MaxSkateSpeed =  3.0f;

	if (Input.y > 0)
	{
		SkateSpeedForward += SkateAcceleration * dt;
		if (SkateSpeedForward > MaxSkateSpeed)
		{
			SkateSpeedForward = MaxSkateSpeed;
		}
	}
	else
	{
		SkateSpeedForward -= SkateDecceleration * dt;
		if (SkateSpeedForward < 0.0f)
		{
			SkateSpeedForward = 0;
		}
	}

	glm::vec4 Movement = (SkateSpeedForward * SkatingPlayerForwardDirection) + VelPerpendicularToGround;

	glm::vec3 Vel(Movement.x, Movement.y, Movement.z);
	GetParent()->GetPhysics()->SetVelocity(&Vel);
	TraceMessage("Velocity watch", "velocity y: %f", Vel.y);
	
}

void BehaviorPlayer::CameraControl(float dt)
{
	glm::vec4 CurrCamPos = *(GetCurrentScene()->GetCamera()->GetPos());
	glm::vec4 PlayerPosition = GetParent()->GetTransform()->GetPosition();
	GetCurrentScene()->GetCamera()->SetPlayerPos(&PlayerPosition);
	glm::vec3 CameraInput = GetInputArrowKeys();//while the player used wasd, camera uses arrow keys
	if (CameraInput.x < 0)
	{
		GetCurrentScene()->GetCamera()->RotateOnUpAxisAroundPoint(-1, &PlayerPosition);
	}
	if (CameraInput.x > 0)
	{
		GetCurrentScene()->GetCamera()->RotateOnUpAxisAroundPoint(1, &PlayerPosition);
	}
	float CamDistFromPlayer = 10;
	glm::vec4 CameraOffsetFromPlayer = CamDistFromPlayer * -1.0f * *(GetCurrentScene()->GetCamera()->GetLookVec());
	glm::vec4 FinalCameraPos = PlayerPosition + CameraOffsetFromPlayer;
	FinalCameraPos.y = PlayerPosition.y + 2.0f;
	GetCurrentScene()->GetCamera()->SetPos(FinalCameraPos);
}

void BehaviorPlayer::SpriteRotation()//the game object is a 3d cube, but there is a sprite that moves with it to display the player character
{
	//the purpose of this function is to display the correct sprite based on where the player is facing relative to the camera
	glm::vec4 CamLooking = *(GetCurrentScene()->GetCamera()->GetLookVec());
	glm::vec4 PlayerLooking = SkatingPlayerForwardDirection;
	float Dot = (glm::dot(CamLooking, PlayerLooking));
	PlayerSpriteObject->GetTransform()->SetRotation(0, 0, 0);
	
	if (Dot > 0.5)
	{
		PlayerSpriteObject->SetTexture("flossy_back.png");
	}
	else if (Dot < -0.5)
	{
		PlayerSpriteObject->SetTexture("flossy_front.png");
	}

	else
	{
		PlayerSpriteObject->GetTransform()->SetRotation(0, 90.0f, 0);
		PlayerSpriteObject->SetTexture("flossy_side.png");
	}
}

void BehaviorPlayer::ChangeUpDirection(glm::vec4* NewUp, float AngleOfRotation)
{
	glm::vec4 AxisOfRotation = glm::normalize(Aff_Cross(CurrentUpDirection, *NewUp));
	glm::mat4x4 RotationMatrix = Aff_Rotate(AngleOfRotation, AxisOfRotation);
	glm::mat4x4 NewRotation = RotationMatrix * GetParent()->GetTransform()->GetOrientationMatrix();
	GetParent()->GetTransform()->SetOrientationMatrix(NewRotation);
	CurrentUpDirection = *NewUp;
	SkatingPlayerForwardDirection = NewRotation * glm::vec4(0, 0, 1, 0);
}

BehaviorPlayer::~BehaviorPlayer()
{

}