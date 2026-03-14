// Check collision between two circles (optimized version)
bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2)
{
    float dx = center2.x - center1.x;
    float dy = center2.y - center1.y;
    float sumRadius = radius1 + radius2;
    
    return (dx*dx + dy*dy) <= (sumRadius*sumRadius);
}