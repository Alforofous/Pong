#include "Ball.h"

#include <SFML/Graphics.hpp>
#include "Game.h"
#include "Pitch.h"
#include "Paddle.h"
#include "MathUtils.h"
#include <math.h>

#include "Constants.h"

Ball::Ball(Game* pGame)
    : m_pGame(pGame)
{
    
}

Ball::~Ball()
{
    
}

bool Ball::initialise()
{
    return true;
}

bool sphere_rectangle_collision(sf::CircleShape sphere, sf::RectangleShape rect)
{
    sf::Vector2f    spherePos;
    sf::Vector2f    rectPos;
    sf::Vector2f    rectSize;
    sf::Vector2f    rectHalfSize;
    float           sphereRadius;

    spherePos = sphere.getPosition();
    sphereRadius = sphere.getRadius();

    rectPos = rect.getPosition();
    rectSize = rect.getSize();
    rectHalfSize.x = rectSize.x / 2.0f;
    rectHalfSize.y = rectSize.y / 2.0f;

    sf::Vector2f delta(
        abs(spherePos.x - (rectPos.x + rectHalfSize.x)),
        abs(spherePos.y - (rectPos.y + rectHalfSize.y))
    );
    if (delta.x > (rectHalfSize.x + sphereRadius))
        return (false);
    if (delta.y > (rectHalfSize.y + sphereRadius))
        return (false);
    if (delta.x <= rectHalfSize.x)
        return (true);
    if (delta.y <= rectHalfSize.y)
        return (true);
    float cornerDistance_sq = pow(delta.x - rectHalfSize.x, 2) +
        pow(delta.y - rectHalfSize.y, 2);
    return (cornerDistance_sq <= pow(sphereRadius, 2));
}

static void    check_for_goal(Game* game, float newPosition, float BallRadius, float pitchSize)
{
    if (newPosition + BallRadius < 0.f)
    {
        game->scoreGoal(Side::RIGHT);
    }
    else if (newPosition - BallRadius > pitchSize)
    {
        game->scoreGoal(Side::LEFT);
    }
}

static sf::Vector2f    check_against_pitch(sf::Vector2f newPosition, float BallRadius, float pitchSize, float *y_velocity)
{
    float ballTopPosition = newPosition.y - BallRadius;
    if (ballTopPosition < 0.f)
    {
        newPosition.y += ballTopPosition * 2.f;
        *y_velocity *= -1;
    }

    float ballBottomPosition = newPosition.y + BallRadius;
    if (ballBottomPosition > pitchSize)
    {
        newPosition.y -= (ballBottomPosition - pitchSize) * 2.f;
        *y_velocity *= -1;
    }

    newPosition.y = std::clamp(newPosition.y, 0.01f + BallRadius, pitchSize - 0.01f - BallRadius);
    return (newPosition);
}

static sf::Vector2f bounce_off_paddle(const Paddle* paddle, sf::Vector2f ball_center)
{
    sf::Vector2f    paddle_position;
    sf::Vector2f    half_paddle_size;
    sf::Vector2f    paddle_center;
    sf::Vector2f    velocity;

    paddle_position.x = paddle->getRect().left;
    paddle_position.y = paddle->getRect().top;
    half_paddle_size.x = PaddleWidth / 2;
    half_paddle_size.y = PaddleHeight / 2;
    paddle_center = paddle_position + half_paddle_size;
    velocity = normalize_vector(ball_center - paddle_center);
    velocity = scale_vector(velocity, 400);
    return (velocity);
}

bool Ball::check_against_paddles(sf::Vector2f newPosition)
{
    sf::CircleShape     sphere;
    sf::RectangleShape  rectangle;

    sphere.setPosition(newPosition);
    sphere.setRadius(BallRadius);
    rectangle = sf::RectangleShape(sf::Vector2f(PaddleWidth, PaddleHeight));
    const sf::Vector2f  pitchSize = m_pGame->getPitch()->getPitchSize();
    const Side          sideToTest = (newPosition.x < pitchSize.x * 0.5f) ? Side::LEFT : Side::RIGHT;
    const Paddle* paddle = m_pGame->getPaddle(sideToTest);
    const sf::FloatRect paddleRect = paddle->getRect();

    rectangle.setPosition(paddleRect.left, paddleRect.top);
    if (sphere_rectangle_collision(sphere, rectangle))
    {
        team = sideToTest;
        //printf("rectangle: left%f top%f lenght x%f y%f\n", rectangle.getPosition().x, rectangle.getPosition().y, rectangle.getSize().x, rectangle.getSize().y);
        return (true);
    }
    return (false);
}

static sf::RectangleShape   get_block(sf::Vector2i index)
{
    sf::RectangleShape  block;
    sf::Vector2f        block_size;
    sf::Vector2f        block_position;
    int                 block_squish;
    int                 offset;

    block_squish = 10;
    block_size.x = (SCREEN_X) / BLOCK_WIDTH;
    block_size.y = (SCREEN_X) / block_squish / BLOCK_HEIGHT;
    block.setSize(block_size);
    block.setOutlineColor(sf::Color::Black);
    block.setOutlineThickness(1);
    offset = (SCREEN_Y)-block_size.y * (BLOCK_HEIGHT);
    block_position.x = block_size.x * index.x;
    if (index.y < BLOCK_HEIGHT / 2)
    {
        block.setFillColor(sf::Color::Red);
        block_position.y = block_size.y * index.y;
    }
    else
    {
        block.setFillColor(sf::Color::Blue);
        block_position.y = block_size.y * index.y + offset;
    }
    block.setPosition(block_position);
    return (block);
}

static int first_free_block_vertically(Game* game, sf::Vector2i index)
{
    if (index.y < BLOCK_HEIGHT / 2)
    {
        index.y = BLOCK_HEIGHT - 1;
        while (index.y >= BLOCK_HEIGHT / 2)
        {
            if (game->block[index.y][index.x] == 0)
                return (index.y);
            index.y -= 1;
        }
    }
    else
    {
        index.y = 0;
        while (index.y < BLOCK_HEIGHT / 2)
        {
            if (game->block[index.y][index.x] == 0)
                return (index.y);
            index.y += 1;
        }
    }
    return (-1);
}

void    check_against_blocks(Game *game, sf::Vector2f ball_center, sf::Vector2f *velocity, int ball_team)
{
    sf::CircleShape     sphere;
    sf::RectangleShape  block;
    sf::Vector2i        index;

    sphere.setPosition(ball_center);
    sphere.setRadius(BallRadius);
    index.y = 0;
    while (index.y < BLOCK_HEIGHT)
    {
        index.x = 0;
        while (index.x < BLOCK_WIDTH)
        {
            block = get_block(index);
            if (game->block[index.y][index.x] == 1 && sphere_rectangle_collision(sphere, block))
            {
                if (ball_center.y > SCREEN_Y / 2 && velocity->y > 0)
                    velocity->y *= -1;
                if (ball_center.y < SCREEN_Y / 2 && velocity->y < 0)
                    velocity->y *= -1;
                if (ball_team == LEFT && block.getFillColor() == sf::Color::Blue)
                {
                    game->block[index.y][index.x] = 0;
                    game->block[first_free_block_vertically(game, index)][index.x] = 1;
                }
                if (ball_team == RIGHT && block.getFillColor() == sf::Color::Red)
                {
                    game->block[index.y][index.x] = 0;
                    game->block[first_free_block_vertically(game, index)][index.x] = 1;
                }
                return ;
            }
            index.x += 1;
        }
        index.y += 1;
    }
}

static sf::Vector2f    block_percentage(Game *game)
{
    sf::Vector2i    index;
    sf::Vector2f    red_vs_blue;

    index.y = 0;
    red_vs_blue.x = 0;
    red_vs_blue.y = 0;
    while (index.y < BLOCK_HEIGHT)
    {
        index.x = 0;
        while (index.x < BLOCK_WIDTH)
        {
            if (game->block[index.y][index.x] == 1)
            {
                if (index.y < BLOCK_HEIGHT / 2)
                    red_vs_blue.x += 1;
                else
                    red_vs_blue.y += 1;
            }
            index.x += 1;
        }
        index.y += 1;
    }
    red_vs_blue.x /= BLOCK_HEIGHT * BLOCK_WIDTH;
    red_vs_blue.y /= BLOCK_HEIGHT * BLOCK_WIDTH;
    return (red_vs_blue);
}

void Ball::update(float deltaTime)
{
    if (m_pGame->getState() == Game::State::WAITING)
    {
        // don't update the ball when we wait.
        return;
    }

    // check ball against boundaries of the pitch
    sf::Vector2f    lastPosition = getPosition();
    float           percentage;

    percentage = 1;
    if (team == LEFT)
        percentage = block_percentage(m_pGame).x * 4;
    else if (team == RIGHT)
        percentage = block_percentage(m_pGame).y * 4;
    percentage = powf(percentage + 2, 2);
    percentage -= 8;
    percentage = fmax(percentage, 1.0);
    printf("percentage %f\n", percentage);
    sf::Vector2f    newPosition = lastPosition + m_velocity * (deltaTime * percentage);
    
    const sf::Vector2f  pitchSize = m_pGame->getPitch()->getPitchSize();
    const Side          sideToTest = (newPosition.x < pitchSize.x * 0.5f) ? Side::LEFT : Side::RIGHT;
    const Paddle*       paddle = m_pGame->getPaddle(sideToTest);

    newPosition = check_against_pitch(newPosition, BallRadius, pitchSize.y, &m_velocity.y);

    temp_ball_center = lastPosition;
    temp_ball_destination = newPosition;

    // check ball against paddles
    if (check_against_paddles(newPosition))
    {
        m_velocity = bounce_off_paddle(paddle, getPosition());
    }
    check_against_blocks(m_pGame, newPosition, &m_velocity, team);
    setPosition(newPosition);
    // check for goal
    check_for_goal(m_pGame, newPosition.x, BallRadius, pitchSize.x);
}

static void draw_blocks(Game *game, sf::RenderTarget& target)
{
    sf::Vector2i        index;
    sf::RectangleShape  block;

    index.y = 0;
    while (index.y < BLOCK_HEIGHT)
    {
        index.x = 0;
        while (index.x < BLOCK_WIDTH)
        {
            block = get_block(index);
            if (game->block[index.y][index.x] == 1)
                target.draw(block);
            index.x += 1;
        }
        index.y += 1;
    }
}

void Ball::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    if (m_pGame->getState() == Game::State::WAITING)
    {
        // don't render the ball when we wait.
        return;
    }
    draw_blocks(m_pGame, target);
    sf::CircleShape circle(BallRadius, BallPoints);
    if (team == LEFT)
        circle.setFillColor(sf::Color::Red);
    else if (team == RIGHT)
        circle.setFillColor(sf::Color::Blue);
    else
        circle.setFillColor(sf::Color::White);
    circle.setPosition(getPosition()-sf::Vector2f(BallRadius, BallRadius));
    target.draw(circle);
}

void Ball::fireFromCenter()
{
    sf::Vector2f pitchSize = m_pGame->getPitch()->getPitchSize();
    setPosition(pitchSize*0.5f);
    
    // choose random direction
    team = -1;
    float randomAngle = (rand() % 1000) * 0.001f * 3.14159265359 * 2.f;
    m_velocity.x = sinf(randomAngle) * FiringSpeed;
    m_velocity.y = cosf(randomAngle) * FiringSpeed;
}
