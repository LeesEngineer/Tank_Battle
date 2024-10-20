#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>

// 子弹类
class Bullet {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;

    Bullet(sf::Vector2f startPosition, float angle) {
        shape.setRadius(5.f);
        shape.setFillColor(sf::Color::Black);
        shape.setPosition(startPosition);

        // 根据发射角度设置子弹速度
        float speed = 600.f; // 子弹速度
        velocity.x = std::cos(angle * 3.14159 / 180.f) * speed;
        velocity.y = std::sin(angle * 3.14159 / 180.f) * speed;
    }

    void update(float deltaTime) {
        shape.move(velocity * deltaTime);
    }

    bool isOutsideWindow(const sf::RenderWindow &window) const {
        return shape.getPosition().x < 0 || shape.getPosition().x > window.getSize().x ||
               shape.getPosition().y < 0 || shape.getPosition().y > window.getSize().y;
    }

    void draw(sf::RenderWindow &window) {
        window.draw(shape);
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
};

// 坦克类
class Tank {
public:
    sf::RectangleShape body;
    sf::RectangleShape cannon;
    sf::Vector2f position;
    float speed;
    float rotationSpeed;
    float lastShotTime; // 记录上次发射的时间
    int health; // 血量
    std::vector<Bullet> bullets; // 存储子弹的容器

    Tank(sf::Vector2f startPos, sf::Color color) {
        position = startPos;
        speed = 100.f;
        rotationSpeed = 100.f;
        lastShotTime = 0.f; // 初始化上次发射时间
        health = 3; // 每辆坦克有 3 点血量

        // 坦克主体
        body.setSize(sf::Vector2f(50.f, 30.f));
        body.setOrigin(25.f, 15.f);
        body.setFillColor(color);
        body.setPosition(position);

        // 坦克炮管
        cannon.setSize(sf::Vector2f(30.f, 5.f));
        cannon.setOrigin(0.f, 2.5f);
        cannon.setFillColor(sf::Color::Black);
        cannon.setPosition(position);
    }

    // 坦克被击中时调用，减少血量
    void takeDamage() {
        if (health > 0) {
            health -= 1;
            if (health == 0) {
                body.setFillColor(sf::Color::Black); // 血量为0时坦克变黑
            }
        }
    }

    // 更新函数，检查边界和碰撞（包括墙壁）
    void update(float deltaTime, bool forward, bool backward, bool rotateLeft, bool rotateRight, const sf::RenderWindow &window, Tank &otherTank, const std::vector<sf::RectangleShape> &walls) {
        // 坦克血量为 0 时不再移动
        if (health == 0) return;

        float angle = body.getRotation();
        sf::Vector2f newPosition = position; // 计算新的位置但不立即应用

        if (forward) {
            newPosition.x += std::cos(angle * 3.14159 / 180.f) * speed * deltaTime;
            newPosition.y += std::sin(angle * 3.14159 / 180.f) * speed * deltaTime;
        }
        if (backward) {
            newPosition.x -= std::cos(angle * 3.14159 / 180.f) * speed * deltaTime;
            newPosition.y -= std::sin(angle * 3.14159 / 180.f) * speed * deltaTime;
        }

        // 边界检测
        float halfWidth = body.getSize().x / 2.f;
        float halfHeight = body.getSize().y / 2.f;

        // 防止坦克超出屏幕边界
        if (newPosition.x - halfWidth < 0.f)
            newPosition.x = halfWidth;
        if (newPosition.x + halfWidth > window.getSize().x)
            newPosition.x = window.getSize().x - halfWidth;
        if (newPosition.y - halfHeight < 0.f)
            newPosition.y = halfHeight;
        if (newPosition.y + halfHeight > window.getSize().y)
            newPosition.y = window.getSize().y - halfHeight;

        // 检查与墙的碰撞
        body.setPosition(newPosition); // 暂时设置位置，看看是否与其他物体碰撞
        bool collided = false;
        for (const auto &wall : walls) {
            if (body.getGlobalBounds().intersects(wall.getGlobalBounds())) {
                collided = true;
                break;
            }
        }
        if (body.getGlobalBounds().intersects(otherTank.body.getGlobalBounds())) {
            collided = true;
        }

        if (collided) {
            // 如果发生碰撞，回到原来的位置，不应用新的位置
            body.setPosition(position);
        } else {
            // 如果没有碰撞，更新位置
            position = newPosition;
        }

        if (rotateLeft) {
            body.rotate(-rotationSpeed * deltaTime);
            cannon.rotate(-rotationSpeed * deltaTime);
        }
        if (rotateRight) {
            body.rotate(rotationSpeed * deltaTime);
            cannon.rotate(rotationSpeed * deltaTime);
        }

        body.setPosition(position);
        cannon.setPosition(position); // 确保炮管位置与坦克主体保持一致
        cannon.setRotation(body.getRotation()); // 确保炮管指向坦克的前方

        // 更新子弹
        for (size_t i = 0; i < bullets.size(); ) {
            bullets[i].update(deltaTime);
            // 检查子弹与墙壁的碰撞
            bool bulletCollided = false;
            for (const auto &wall : walls) {
                if (bullets[i].getBounds().intersects(wall.getGlobalBounds())) {
                    bulletCollided = true;
                    break;
                }
            }

            // 检查子弹与坦克的碰撞
            if (bullets[i].getBounds().intersects(otherTank.body.getGlobalBounds())) {
                // 子弹击中对方坦克时，减少其血量
                otherTank.takeDamage();
                bulletCollided = true; // 子弹与坦克相撞，也将其视为碰撞
            }

            if (bullets[i].isOutsideWindow(window) || bulletCollided) {
                bullets.erase(bullets.begin() + i); // 移除超出窗口或与墙壁或坦克碰撞的子弹
            } else {
                ++i;
            }
        }

        lastShotTime += deltaTime; // 更新发射计时器
    }

    void shoot() {
        if (lastShotTime >= 0.5f && health > 0) { // 仅在血量大于0时能发射子弹
            float angle = cannon.getRotation(); // 使用炮管的角度发射子弹
            // 子弹从炮管末端发射，计算炮管的末端位置
            sf::Vector2f cannonEnd = cannon.getPosition() + sf::Vector2f(std::cos(angle * 3.14159 / 180.f) * cannon.getSize().x,
                                                                        std::sin(angle * 3.14159 / 180.f) * cannon.getSize().x);
            bullets.emplace_back(cannonEnd, angle); // 在炮管末端位置发射子弹
            lastShotTime = 0.f; // 重置发射计时器
        }
    }

    void draw(sf::RenderWindow &window) {
        window.draw(body);
        window.draw(cannon);
        for (auto &bullet : bullets) {
            bullet.draw(window);
        }
    }
};

// 主函数
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Tank Battle");

    // 创建两辆坦克，确保坦克在墙壁附近
    Tank tank1(sf::Vector2f(150.f, 300.f), sf::Color::Green); // 离墙壁更远一点
    Tank tank2(sf::Vector2f(650.f, 300.f), sf::Color::Red); // 离墙壁更远一点

    // 创建墙壁作为掩护
    std::vector<sf::RectangleShape> walls;

    // 增加墙壁数量到10个，布局如下，离窗口边缘远些
    float wallOffset = 50.f; // 墙壁与窗口边缘的距离

    sf::RectangleShape wall1(sf::Vector2f(100.f, 20.f)); // 上方左
    wall1.setPosition(100.f + wallOffset, 50.f + wallOffset);
    wall1.setFillColor(sf::Color::Blue);
    walls.push_back(wall1);

    sf::RectangleShape wall2(sf::Vector2f(100.f, 20.f)); // 上方中
    wall2.setPosition(300.f + wallOffset, 50.f + wallOffset);
    wall2.setFillColor(sf::Color::Blue);
    walls.push_back(wall2);

    sf::RectangleShape wall3(sf::Vector2f(100.f, 20.f)); // 上方右
    wall3.setPosition(500.f + wallOffset, 50.f + wallOffset);
    wall3.setFillColor(sf::Color::Blue);
    walls.push_back(wall3);

    sf::RectangleShape wall4(sf::Vector2f(20.f, 100.f)); // 左侧
    wall4.setPosition(50.f + wallOffset, 150.f + wallOffset);
    wall4.setFillColor(sf::Color::Blue);
    walls.push_back(wall4);

    sf::RectangleShape wall5(sf::Vector2f(20.f, 100.f)); // 右侧
    wall5.setPosition(730.f - wallOffset, 150.f + wallOffset);
    wall5.setFillColor(sf::Color::Blue);
    walls.push_back(wall5);

    sf::RectangleShape wall6(sf::Vector2f(100.f, 20.f)); // 下方左
    wall6.setPosition(100.f + wallOffset, 500.f - wallOffset);
    wall6.setFillColor(sf::Color::Blue);
    walls.push_back(wall6);

    sf::RectangleShape wall7(sf::Vector2f(100.f, 20.f)); // 下方中
    wall7.setPosition(300.f + wallOffset, 500.f - wallOffset);
    wall7.setFillColor(sf::Color::Blue);
    walls.push_back(wall7);

    sf::RectangleShape wall8(sf::Vector2f(100.f, 20.f)); // 下方右
    wall8.setPosition(500.f + wallOffset, 500.f - wallOffset);
    wall8.setFillColor(sf::Color::Blue);
    walls.push_back(wall8);

    sf::RectangleShape wall9(sf::Vector2f(20.f, 100.f)); // 中左
    wall9.setPosition(150.f + wallOffset, 300.f + wallOffset);
    wall9.setFillColor(sf::Color::Blue);
    walls.push_back(wall9);

    sf::RectangleShape wall10(sf::Vector2f(20.f, 100.f)); // 中右
    wall10.setPosition(650.f - wallOffset, 300.f + wallOffset);
    wall10.setFillColor(sf::Color::Blue);
    walls.push_back(wall10);
    
    // 中心墙
    sf::RectangleShape wall11(sf::Vector2f(200.f, 20.f)); // 增加宽度至200.f
    wall11.setPosition(300.f, 250.f); // 减小x坐标，向左移动墙
    wall11.setFillColor(sf::Color::Blue);
    walls.push_back(wall11);


    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float deltaTime = clock.restart().asSeconds();

        // 检查输入
        bool moveForward1 = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
        bool moveBackward1 = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
        bool rotateLeft1 = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
        bool rotateRight1 = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
        bool shoot1 = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

        bool moveForward2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        bool moveBackward2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        bool rotateLeft2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        bool rotateRight2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        bool shoot2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Enter);

        tank1.update(deltaTime, moveForward1, moveBackward1, rotateLeft1, rotateRight1, window, tank2, walls);
        tank2.update(deltaTime, moveForward2, moveBackward2, rotateLeft2, rotateRight2, window, tank1, walls);

        if (shoot1) {
            tank1.shoot();
        }
        if (shoot2) {
            tank2.shoot();
        }

        window.clear(sf::Color::White);

        // 绘制坦克
        tank1.draw(window);
        tank2.draw(window);

        // 绘制墙壁
        for (const auto &wall : walls) {
            window.draw(wall);
        }

        window.display();
    }

    return 0;
}

