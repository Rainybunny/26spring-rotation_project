// Check collision between two circles
bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2)
{
    float dx = center2.x - center1.x;      // X distance between centers
    float dy = center2.y - center1.y;      // Y distance between centers
    float sumRadii = radius1 + radius2;    // Sum of radii
    
    // Compare squared distances to avoid sqrtf()
    return (dx*dx + dy*dy) <= (sumRadii*sumRadii);
}