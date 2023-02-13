#pragma once

#include <math.h>
#include <SFML/System.hpp>

float VecLength(sf::Vector2f a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
}

sf::Vector2f    normalize_vector(sf::Vector2f vector)
{
    float   w;

    w = (float)sqrt(vector.x * vector.x + vector.y * vector.y);
    vector.x /= w;
    vector.y /= w;
    return (vector);
}

sf::Vector2f    scale_vector(sf::Vector2f vector, double scalar)
{
    vector.x *= scalar;
    vector.y *= scalar;
    return (vector);
}