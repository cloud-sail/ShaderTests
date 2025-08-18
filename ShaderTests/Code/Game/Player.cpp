#include "Game/Player.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"

Player::Player(Game* owner)
	: Entity(owner)
{
	m_position = Vec3(-2.f, 0.f, 1.f);

	float aspect = g_gameConfigBlackboard.GetValue("windowAspect", 1.777f);
	m_camera.SetPerspectiveView(aspect, 60.f, 0.1f, 100.0f);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);

}

void Player::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	float unscaledDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
	UpdateOrientation(unscaledDeltaSeconds);
	UpdatePosition(unscaledDeltaSeconds);

	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_H) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}
	UpdateCamera();
}

void Player::Render() const
{

}

void Player::UpdateOrientation(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);

	// Yaw & Pitch
	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
	float deltaYaw = -cursorPositionDelta.x * 0.125f;
	float deltaPitch = cursorPositionDelta.y * 0.125f;

	m_orientation.m_yawDegrees += deltaYaw;
	m_orientation.m_pitchDegrees += deltaPitch;

	Vec2 rightStick = controller.GetRightStick().GetPosition();
	deltaYaw = -deltaSeconds * CAMERA_YAW_TURN_RATE * rightStick.x;
	deltaPitch = -deltaSeconds * CAMERA_PITCH_TURN_RATE * rightStick.y;

	m_orientation.m_yawDegrees += deltaYaw;
	m_orientation.m_pitchDegrees += deltaPitch;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);

	// Roll
	float posRoll = (g_theInput->IsKeyDown(KEYCODE_C) || controller.GetRightTrigger() > 0.f) ? 1.f : 0.f;
	float negRoll = (g_theInput->IsKeyDown(KEYCODE_Z) || controller.GetLeftTrigger() > 0.f) ? -1.f : 0.f;
	float deltaRoll = (posRoll + negRoll) * deltaSeconds * CAMERA_ROLL_TURN_RATE;
	m_orientation.m_rollDegrees += deltaRoll;
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -CAMERA_MAX_ROLL, CAMERA_MAX_ROLL);

}

void Player::UpdatePosition(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);

	float const speedMultiplier = (g_theInput->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_A)) ? CAMERA_SPEED_FACTOR : 1.f;

	Vec3 moveIntention = Vec3(controller.GetLeftStick().GetPosition().GetRotatedMinus90Degrees());

	if (g_theInput->IsKeyDown(KEYCODE_W))
	{
		moveIntention += Vec3(1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		moveIntention += Vec3(-1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_A))
	{
		moveIntention += Vec3(0.f, 1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		moveIntention += Vec3(0.f, -1.f, 0.f);
	}

	moveIntention.ClampLength(1.f);

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);
	m_position += (forwardIBasis * moveIntention.x + leftJBasis * moveIntention.y + upKBasis * moveIntention.z) * 
		CAMERA_MOVE_SPEED * deltaSeconds * speedMultiplier;

	Vec3 elevateIntention;
	if (g_theInput->IsKeyDown(KEYCODE_Q))
	{
		elevateIntention += Vec3(0.f, 0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_E))
	{
		elevateIntention += Vec3(0.f, 0.f, 1.f);
	}
	if (controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER))
	{
		elevateIntention += Vec3(0.f, 0.f, -1.f);
	}
	if (controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_RIGHT_SHOULDER))
	{
		elevateIntention += Vec3(0.f, 0.f, 1.f);
	}
	elevateIntention.ClampLength(1.f);

	m_position += elevateIntention * CAMERA_MOVE_SPEED * deltaSeconds * speedMultiplier;
}

void Player::UpdateCamera()
{
	m_camera.SetPositionAndOrientation(m_position, m_orientation);
}
