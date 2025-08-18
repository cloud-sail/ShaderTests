#include "Game/Entity.hpp"
#include "Engine/Math/Mat44.hpp"

Entity::Entity(Game* owner)
	: m_game(owner)
{

}

Mat44 Entity::GetModelToWorldTransform() const
{
	Mat44 result = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	result.AppendScaleNonUniform3D(m_scale);
	result.SetTranslation3D(m_position);
	return result;
}
