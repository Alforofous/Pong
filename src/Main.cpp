#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include "Game.h"
#include <memory>
#include <iostream>

void	init_window(sf::RenderWindow* window, const char* window_name)
{
    sf::Vector2u	window_size;
    sf::Vector2i	window_position;

    window_size.x = SCREEN_X;
    window_size.y = SCREEN_Y;
    window_position.x = 0 + window_size.x / 2;
    window_position.y = 0 + window_size.y / 2;
    window->setSize(window_size);
    window->setPosition(window_position);
    window->create(sf::VideoMode(window_size.x, window_size.y), window_name);
}

int main()
{
    sf::RenderWindow window;
    window.setKeyRepeatEnabled(false);
    
    init_window(&window, "hive_test");
    std::unique_ptr<Game> pGame = std::make_unique<Game>();
    if (!pGame->initialise(window.getView().getSize()))
    {
        std::cerr << "Game Failed to initialise" << std::endl;
        return 1;
    }
    
    sf::Clock clock;
    
    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    // "close requested" event: we close the window
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    pGame->onKeyPressed(event.key.code);
                    break;
                case sf::Event::KeyReleased:
                    pGame->onKeyReleased(event.key.code);
                    break;
                default:
                    break;
            }
        }
        
        sf::Time elapsedTime = clock.getElapsedTime();
        clock.restart();
        pGame->update(elapsedTime.asSeconds());
        
        // clear the window with black color
        window.clear(sf::Color::Black);
        
        window.draw(*pGame.get());
        
        // end the current frame
        window.display();
    }
    
    return 0;
}
