#include "MovieLoader.cpp"
#include <vector>

class MockMovieLoader : public MovieLoader {
  public:
    explicit MockMovieLoader() {}

    std::vector<Movie> load() override {
        std::vector<Movie> movies;

        Movie m1{1,
                 1901,
                 "Kansas Saloon Smashers",
                 "American",
                 "Unknown",
                 "",
                 "unknown",
                 "https://en.wikipedia.org/wiki/Kansas_Saloon_Smashers",
                 "A bartender is working at a saloon, serving drinks to customers. "
                 "After he fills a stereotypically Irish man's bucket with beer, "
                 "Carrie Nation and her followers burst inside. They assault the "
                 "Irish man, pulling his hat over his eyes and then dumping the beer "
                 "over his head. The group then begin wrecking the bar, smashing the "
                 "fixtures, mirrors, and breaking the cash register. The bartender "
                 "then sprays seltzer water in Nation's face before a group of "
                 "policemen appear and order everybody to leave.[1]",
                 {}};

        movies.push_back(m1);
        Movie m2{2,
                 1901,
                 "Love by the Light of the Moon",
                 "American",
                 "Unknown",
                 "",
                 "unknown",
                 "https://en.wikipedia.org/wiki/Love_by_the_Light_of_the_Moon",
                 "The moon, painted with a smiling face hangs over a park at night. A young couple "
                 "walking past a fence learn on a railing and look up. The moon smiles. They "
                 "embrace, and the moon's smile gets bigger. They then sit down on a bench by a "
                 "tree. The moon's view is blocked, causing him to frown. In the last scene, the "
                 "man fans the woman with his hat because the moon has left the sky and is perched "
                 "over her shoulder to see everything better.",
                 {}};

        movies.push_back(m2);
        Movie m3{3,
                 1901,
                 "The Martyred Presidents",
                 "American",
                 "Unknown",
                 "",
                 "unknown",
                 "https://en.wikipedia.org/wiki/The_Martyred_Presidents",
                 "The film, just over a minute long, is composed of two shots. In the first, a "
                 "girl sits at the base of an altar or tomb, her face hidden from the camera. At "
                 "the center of the altar, a viewing portal displays the portraits of three U.S. "
                 "Presidents—Abraham Lincoln, James A. Garfield, and William McKinley—each victims "
                 "of assassination. In the second shot, which runs just over eight seconds long, "
                 "an assassin kneels feet of Lady Justice.",
                 {}};

        movies.push_back(m3);
        Movie m4{
            4,
            1901,
            "Terrible Teddy, the Grizzly King",
            "American",
            "Unknown",
            "",
            "unknown",
            "https://en.wikipedia.org/wiki/Terrible_Teddy,_the_Grizzly_King",
            "Lasting just 61 seconds and consisting of two shots, the first shot is set in a wood "
            "during winter. The actor representing then vice-president Theodore Roosevelt "
            "enthusiastically hurries down a hillside towards a tree in the foreground. He falls "
            "once, but rights himself and cocks his rifle. Two other men, bearing signs reading "
            "\"His Photographer\" and \"His Press Agent\" respectively, follow him into the shot; "
            "the photographer sets up his camera. \"Teddy\" aims his rifle upward at the tree and "
            "fells what appears to be a common house cat, which he then proceeds to stab. "
            "\"Teddy\" holds his prize aloft, and the press agent takes notes. The second shot is "
            "taken in a slightly different part of the wood, on a path. \"Teddy\" rides the path "
            "on his horse towards the camera and out to the left of the shot, followed closely by "
            "the press agent and photographer, still dutifully holding their signs.",
            {}};

        movies.push_back(m4);
        Movie m5{
            5,
            1902,
            "Jack and the Beanstalk",
            "American",
            "George S. Fleming, Edwin S. Porter",
            "",
            "unknown",
            "https://en.wikipedia.org/wiki/Jack_and_the_Beanstalk_(1902_film)",
            "The earliest known adaptation of the classic fairytale, this films shows Jack trading "
            "his cow for the beans, his mother forcing him to drop them in the front yard, and "
            "beig forced upstairs. As he sleeps, Jack is visited by a fairy who shows him glimpses "
            "of what will await him when he ascends the bean stalk. In this version, Jack is the "
            "son of a deposed king. When Jack wakes up, he finds the beanstalk has grown and he "
            "climbs to the top where he enters the giant's home. The giant finds Jack, who "
            "narrowly escapes. The giant chases Jack down the bean stalk, but Jack is able to cut "
            "it down before the giant can get to safety. He falls and is killed as Jack "
            "celebrates. The fairy then reveals that Jack may return home as a prince.",
            {}};

        movies.push_back(m5);

        return movies;
    }
};