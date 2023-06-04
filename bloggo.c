#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <mysql/mysql.h>


#define SERVER "localhost"
#define USER "root"
#define PASSWORD "adya"
#define DATABASE "bloggo"


// General Utilities start here
// To calculate screen width of the terminal window
int getScreenWidth() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, & w);
    return w.ws_col;
}


// To print a string in the center of screen
void printHeading(const char * text) {
    int textLength = strlen(text);
    int screenWidth = getScreenWidth();
    int padding = (screenWidth - textLength) / 2;

    printf("\n");
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }

    printf("\033[1m%s\033[0m\n", text);
}


// To print a divider line across the whole screen
void printLine() {
    int screenWidth = getScreenWidth();

    printf("\n+");
    for (int i = 0; i < screenWidth - 2; i++) {
        printf("-");
    }
    printf("+");
}


// To print the 'bloggo' logo in the center
void printLogo(char * text) {
    int textLength = 78;
    int screenWidth = getScreenWidth();
    int padding = (screenWidth - textLength) / 2;

    for (int i = 0; i < padding; i++) {
        printf(" ");
    }

    printf("%s", text);
}


// To read logo from logo.txt
void logo () {
    FILE *logo = fopen( "logo.txt", "r" );
    char line[300];

    while ( fgets(line, 299, logo) != NULL ) {
        printLogo(line);
    }
    printf("\n");

    fclose(logo);
}
// General Utilities end here


// Main program functions start here
// To display the menu when not signed in
void display_menu() {

    printLine();
    printHeading("\033[4mMenu\033[0m");
    printf("1. Login\n");
    printf("2. Signup\n");
    printf("3. View latest blogs\n");
    printf("4. View blogs by category\n");
    printf("0. Exit\n");
}


// To display the menu when signed in
void display_user_menu() {

    printLine();
    printHeading("\033[4mMenu\033[0m");
    printf("3. View latest blogs\n");
    printf("4. View blogs by category\n");
    printf("5. View my blogs\n");
    printf("6. Add a new blog\n");
    printf("7. Edit a blog\n");
    printf("8. Delete a blog\n");
    printf("9. Logout\n");
}


// Function to login the user by checking SQL database
int login(MYSQL * conn, char * username, char * password) {
    MYSQL_RES * res;
    int status;

    char query[100];
    sprintf(query, "SELECT * FROM users WHERE username='%s' AND password='%s'", username, password);

    // status stores an integer that indicates if the query executed successfully.
    // status = 0 if it was successful, != 0 otherwise
    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return 0;
    }

    // res stores the data that is retrieved from the database after query execution
    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return 0;
    }

    int num_rows = mysql_num_rows(res);
    mysql_free_result(res);

    if (num_rows == 0) {
        return 0;
    }

    return 1;
}


// Function to register a new user and insert into SQL database
int signup(MYSQL * conn, char * username, char * password) {
    int status;

    char query[100];
    sprintf(query, "INSERT INTO users (username, password) VALUES ('%s', '%s')", username, password);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return 0;
    }

    return 1;
}


// Function to update total_likes for a blog in blogs table
void update_total_likes(MYSQL* conn, int blogID, int increment) {
    char query[100];
    sprintf(query, "UPDATE blogs SET total_likes = total_likes + %d WHERE id = %d", increment, blogID);
    int status = mysql_query(conn, query);

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    }
}


// Function to decrement number of likes for a blog
void unlike_blog(MYSQL* conn, int blogID, char* username) {
    char query[100];
    sprintf(query, "DELETE FROM blog_likes WHERE blog_id = %d AND username = '%s'", blogID, username);
    int status = mysql_query(conn, query);

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    }
    else {
        // Update the total_likes count in the blogs table
        update_total_likes(conn, blogID, -1);
    }
}


// Function to increment number of blogs for a blog
void like_blog(MYSQL* conn, int blogID, char* username) {
    char query[100];
    sprintf(query, "INSERT INTO blog_likes (blog_id, username, liked) VALUES (%d, '%s', 1)", blogID, username);
    int status = mysql_query(conn, query);

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    } 
    else {
        // Update the total_likes count in the blogs table
        update_total_likes(conn, blogID, 1);
    }
}


// To check if the user has already liked a blog or not
int is_blog_liked(MYSQL* conn, int blogID, char* username) {
    MYSQL_RES* res;
    MYSQL_ROW row;
    int status;

    char query[100];
    sprintf(query, "SELECT * FROM blog_likes WHERE blog_id = %d AND username = '%s'", blogID, username);
    status = mysql_query(conn, query);

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return 0;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return 0;
    }

    int liked = mysql_num_rows(res) > 0;

    mysql_free_result(res);

    return liked;
}


// Function to view a single blog
void view_single_blog(MYSQL* conn, int blog_id, bool is_logged_in, char* username) {
    MYSQL_RES* res;
    MYSQL_ROW row;
    int status;

    char query[100];
    sprintf(query, "SELECT * FROM blogs WHERE id = %d", blog_id);
    status = mysql_query(conn, query);

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    if ((row = mysql_fetch_row(res)) != NULL) {
        printLine();
        printf("\n\n\033[1mBlog ID:\033[0m %s\n", row[0]);
        printf("\n\033[1mTitle:\033[0m %s\n", row[2]);
        printf("\033[1mAuthor:\033[0m %s\n", row[1]);
        printf("\033[1mCategory:\033[0m %s\n", row[3]);
        printf("\033[1mContent:\033[0m %s\n", row[4]);
        printf("\033[1mTotal Likes:\033[0m %s\n", row[6]);
        //printf("\n");
        if (is_logged_in) {
            // Check if the blog was liked by the user
            if (is_blog_liked(conn, blog_id, username)) {
                printf("You have already liked this blog. Do you want to unlike it? (Y/N): ");
                char choice;
                fflush(stdin);
                scanf(" %c", &choice);

                if (choice == 'Y' || choice == 'y') {
                    unlike_blog(conn, blog_id, username);
                    printf("You have successfully unliked the blog.\n");
                }
            }
            else {
                printf("Do you want to like this blog? (Y/N): ");
                char choice;
                fflush(stdin);
                scanf(" %c", &choice);

                if (choice == 'Y' || choice == 'y') {
                    like_blog(conn, blog_id, username);
                    printf("You have successfully liked the blog.\n");
                }
            }
        }
    }
    else {
        printf("No blog found with the given ID.\n");
    }

    printLine();
    printf("\n\n");

    mysql_free_result(res);
}


// Function to display 10 latest blogs from the blogs table in database
void view_latest_blogs(MYSQL * conn, bool is_logged_in, char * username) {
    MYSQL_RES * res;
    MYSQL_ROW row;
    int status;

    printHeading("\033[3mLatest Blogs\033[0m\n\n");

    // To select 10 blogs from table
    status = mysql_query(conn, "SELECT id, author, title, category, 
                            CONCAT(SUBSTRING_INDEX(content, ' ', 10), '...') AS truncated_content, 
                            total_likes
                            FROM blogs
                            ORDER BY created_at DESC
                            LIMIT 10");

    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    while ((row = mysql_fetch_row(res)) != NULL) {
        printLine();
        printf("\n\n\033[1mBlog ID:\033[0m %s\n", row[0]);
        printf("\n\033[1mTitle:\033[0m %s\n", row[2]);
        printf("\033[1mAuthor:\033[0m %s\n", row[1]);
        printf("\033[1mCategory:\033[0m %s\n", row[3]);
        printf("\033[1mContent:\033[0m %s\n", row[4]);
        printf("\033[1mTotal Likes:\033[0m %s\n", row[5]);
        //printf("\n");
    }
    printLine();
    printf("\n\n");

    mysql_free_result(res);

    // Ask the user for the blog ID to view the complete blog
    int blogID;
    printf("Enter the Blog ID to view the complete blog: ");
    fflush(stdin);
    scanf("%d", &blogID);

    // Call a separate function to view the complete blog
    view_single_blog(conn, blogID, is_logged_in, username);
}


// Function to diplay all distinct categories and view blogs related to entered category
void view_blogs_by_category(MYSQL * conn, bool is_logged_in, char * username) {
    MYSQL_RES * res;
    MYSQL_ROW row;
    int status, num_categories, i;
    char query[200], category[50];

    // Get all distinct categories from the blogs table
    sprintf(query, "SELECT DISTINCT category FROM blogs");

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    num_categories = mysql_num_rows(res);

    // If there are no categories, inform the user and return
    if (num_categories == 0) {
        printf("\nThere are no categories to show.\n");
        mysql_free_result(res);
        return;
    }

    // Show all categories to the user
    printf("\n\033[1mSelect a category to view blogs for:\033[0m\n\n");
    while ((row = mysql_fetch_row(res)) != NULL) {
        printf("\t- %s\n", row[0]);
    }
    mysql_free_result(res);

    // Get user's choice of category
    printf("\n\033[1mEnter category name: \033[0m");
    scanf("%s", category);

    // Get all blogs for the chosen category and display them
    sprintf(query, "SELECT id, author, title, category, CONCAT(SUBSTRING_INDEX(content, ' ', 10), '...')
                     AS truncated_content, total_likes FROM blogs WHERE category = '%s' 
                     ORDER BY created_at DESC", category);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    if (mysql_num_rows(res) == 0) {
        printf("No blogs found for the category '%s'.\n", category);
        mysql_free_result(res);
        return;
    }

    while ((row = mysql_fetch_row(res)) != NULL) {
        printLine();
        printf("\n\n\033[1mBlog ID:\033[0m %s\n", row[0]);
        printf("\n\033[1mTitle:\033[0m %s\n", row[2]);
        printf("\033[1mAuthor:\033[0m %s\n", row[1]);
        printf("\033[1mCategory:\033[0m %s\n", row[3]);
        printf("\033[1mContent:\033[0m %s\n", row[4]);
        printf("\033[1mTotal Likes:\033[0m %s\n", row[5]);
        //printf("\n");
    }
    printLine();
    printf("\n\n");
    mysql_free_result(res);

    // Ask the user for the blog ID to view the complete blog
    int blogID;
    printf("Enter the Blog ID to view the complete blog: ");
    fflush(stdin);
    scanf("%d", &blogID);

    // Validate if the blogID belongs to the chosen category
    sprintf(query, "SELECT id FROM blogs WHERE id = %d AND category = '%s'", blogID, category);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    if (mysql_num_rows(res) == 0) {
        printf("Invalid Blog ID for the chosen category.\n");
        mysql_free_result(res);
        return;
    }

    mysql_free_result(res);

    // Call a separate function to view the complete blog
    view_single_blog(conn, blogID, is_logged_in, username);
}


// Function to view blogs posted by the logged in user
void view_my_blogs(MYSQL * conn, char * username) {
    MYSQL_RES * res;
    MYSQL_ROW row;
    int status;
    printf("\nMy blogs:\n"); // Heading

    char query[1000];
    sprintf(query, "SELECT * FROM blogs WHERE author = '%s' ORDER BY created_at DESC", username);
    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    while ((row = mysql_fetch_row(res)) != NULL) {
        printLine();
        printf("\n\033[1mBlog ID:\033[0m %s\n", row[0]);
        printf("\n\033[1mTitle:\033[0m %s\n", row[2]);
        printf("\033[1mAuthor:\033[0m %s\n", row[1]);
        printf("\033[1mCategory:\033[0m %s\n", row[3]);
        printf("\033[1mContent:\033[0m %s\n", row[4]);
        printf("\033[1mTotal Likes:\033[0m %s\n", row[6]);
        printf("\n");
    }
    printLine();
    printf("\n\n");

    mysql_free_result(res);

}


// Function to insert new blog to the blogs table
void add_blog(MYSQL * conn, char * username) {
    char title[50], category[50], content[1000];
    int status;

    printLine();
    printf("\033[1mEnter Bloggo Title:\033[0m ");
    scanf(" %[^\n]s", title);

    printf("\033[1mEnter Bloggo Category:\033[0m ");
    scanf(" %[^\n]s", category);

    printf("\033[1mEnter Bloggo Content\033[0m (max 100 words)\n");
    scanf(" %[^\n]s", content);

    char query[1200];
    sprintf(query, "INSERT INTO blogs (author, title, category, content) VALUES ('%s', '%s', '%s', '%s')", username, title, category, content);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    } 
    else {
        printHeading("\033[7mBloggo added successfully!\n");
    }
}


// Function to edit the content of a blog already posted by the user
void edit_blog(MYSQL * conn, char * username) {
    MYSQL_RES * res;
    MYSQL_ROW row;

    char blog_id[10], content[1001];
    int status;

    printf("\n\033[1mEnter Bloggo ID to edit:\033[0m ");
    scanf("%s", blog_id);

    char query[1500];
    sprintf(query, "SELECT * FROM blogs WHERE id = %s AND author = '%s'", blog_id, username);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if ((row = mysql_fetch_row(res)) == NULL) {
        printf("\n\n\033[1mYou do not have a Bloggo with ID '%s'\033[0m\n", blog_id);
        mysql_free_result(res);
        return;
    }

    printf("\n\033[1mOld Bloggo content:\033[0m\n%s\n", row[4]); // Assuming content is stored in the 4th column

    printf("\n\033[1mEnter new Bloggo content\033[0m (max 100 words):\n");
    scanf(" %[^\n]s", content);

    sprintf(query, "UPDATE blogs SET content = '%s' WHERE id = %s AND author = '%s'", content, blog_id, username);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    } 
    else {
        printf("\n\033[1mBlog with ID '%s' updated successfully!\033[0m\n", blog_id);
    }

    mysql_free_result(res);
}


// Function to delete a blog posted by the logged in user
void delete_blog(MYSQL * conn, char * username) {
    int blog_id;
    int status;
    printf("\n\033[1mEnter Blog ID to delete:\033[0m ");
    scanf("%d", &blog_id);

    char query[200];
    sprintf(query, "DELETE FROM blogs WHERE id = %d AND author = '%s'", blog_id, username);

    status = mysql_query(conn, query);
    if (status != 0) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    } 
    else {
        if (mysql_affected_rows(conn) > 0) {
            printf("\n\033[1mBlog with ID '%d' deleted successfully!\033[0m\n", blog_id);
        } 
        else {
            printf("\n\033[1mError: Invalid Blog ID or you are not the author of the blog.\033[0m\n");
        }
    }
}


// Main building function
int main() {
    MYSQL * conn;
    int status = 0;
    conn = mysql_init(NULL);
    if (conn == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        exit(1);
    }

    conn = mysql_real_connect(conn, SERVER, USER, PASSWORD, NULL, 0, NULL, 0);
    if (conn == NULL) {
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        exit(1);
    }

    // This query will check if Database bloggo exists or not and create database if it does not exist
    if (mysql_query(conn, "CREATE DATABASE IF NOT EXISTS bloggo")) {
        fprintf(stderr, "mysql_query CREATE DATABASE failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    if (mysql_select_db(conn, DATABASE)) {
        fprintf(stderr, "mysql_select_db failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    // This query will check if users table exists or not and create table if it does not exist
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS users (id INT(11) NOT NULL AUTO_INCREMENT, 
                            username VARCHAR(20) NOT NULL unique, password VARCHAR(20) NOT NULL, 
                            PRIMARY KEY (id))")) {
        fprintf(stderr, "mysql_query CREATE TABLE users failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    // This query will check if blogs table exists or not and create table if it does not exist
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS blogs (id INT(11) NOT NULL AUTO_INCREMENT,
                            author VARCHAR(20) NOT NULL, title VARCHAR(100) NOT NULL,
                            category VARCHAR(50) NOT NULL, content TEXT NOT NULL,
                            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, total_likes INT(11) DEFAULT 0,
                            PRIMARY KEY (id));")) {
        fprintf(stderr, "mysql_query CREATE TABLE blogs failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    // This query will check if blog_likes table exists or not and create table if it does not exist
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS blog_likes (id INT(11) NOT NULL AUTO_INCREMENT,
                            blog_id INT(11) NOT NULL, username VARCHAR(20) NOT NULL,
                            liked TINYINT(1) NOT NULL DEFAULT 0, PRIMARY KEY (id),
                            UNIQUE KEY unique_like (blog_id, username));")) {
        fprintf(stderr, "mysql_query CREATE TABLE blog_likes failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    char option;
    char username[20]; // To store username entered by user
    char password[20]; // To store password entered by user
    bool is_logged_in = false; // set to false initially

    // ASCII logo art starting here
    printLine();
    printLine();

    logo(); // To print the logo at center of the screen
    
    printLine();
    printLine();
    printf("\n\n");
    // ASCII logo art ends here

    while (true) {
        if (!is_logged_in) {
            display_menu(); // If user is not logged in
        } 
        else {
            display_user_menu(); // If user is logged in
        }

        printf("\n\033[1mEnter option: \033[0m");
        scanf(" %c", & option);

        // code to handle menu options goes here
        switch (option) {
        case '1': // '1' to login
            if (is_logged_in) {
                printHeading("\033[3mYou are already logged in!\033[0m\n");
            } 
            else {
                printf("\n\033[1mEnter Username:\033[0m ");
                fflush(stdin);
                scanf("%s", username);

                printf("\033[1mEnter Password:\033[0m ");
                fflush(stdin);
                scanf("%s", password);

                status = login(conn, username, password);
                if (status == 1) {
                    printHeading("\033[3mLogin successful!\033[0m\n");
                    is_logged_in = true;
                } else {
                    printHeading("\033[3mLogin failed. Please try again.\033[0m\n");
                }
            }
            break;

        case '2': // '2' to signup new account
            if (is_logged_in) {
                printHeading("\033[3mYou are already signed up!\033[0m\n");
            } 
            else {
                printf("\n\033[1mEnter Username:\033[0m ");
                fflush(stdin);
                scanf("%s", username);

                printf("\033[1mEnter Password:\033[0m ");
                fflush(stdin);
                scanf("%s", password);

                status = signup(conn, username, password);
                if (status == 1) {
                    printHeading("\033[3mSignup successful!\033[0m\n");
                    is_logged_in = true;
                } else {
                    printHeading("\033[3mSignup failed. Please try again.\033[0m");
                }
            }
            break;

        case '3': // '3' to view latest blogs
            if (!is_logged_in) {
                view_latest_blogs(conn, is_logged_in, NULL);
            } 
            else {
                view_latest_blogs(conn, is_logged_in, username);
            }
            break;

        case '4': // '4' to view blogs by category
            if (!is_logged_in) {
                view_blogs_by_category(conn, is_logged_in, NULL);
            } 
            else {
                view_blogs_by_category(conn, is_logged_in, username);
            }
            break;
            
        case '5': // '5' to view blogs posted by logged in user
            if (!is_logged_in) {
                printHeading("\033[3mPlease Login first.\033[0m\n");
            } 
            else {
                view_my_blogs(conn, username);
            }
            break;

        case '6': // '6' to add a blog posted by the logged in user
            if (!is_logged_in) {
                printHeading("\033[3mPlease Login first.\033[0m\n");
            } 
            else {
                add_blog(conn, username);
            }
            break;

        case '7': // '7' to edit a blog posted by the logged in user
            if (!is_logged_in) {
                printHeading("\033[3mPlease Login first.\033[0m\n");
            } 
            else {
                edit_blog(conn, username);
            }
            break;

        case '8': // '8' to delete blog posted by user
            if (!is_logged_in) {
                printHeading("\033[3mPlease Login first.\033[0m\n");
            } 
            else {
                delete_blog(conn, username);
            }
            break;

        case '9': // '9' to logout
            if (!is_logged_in) {
                printHeading("\033[3mPlease Login first.\033[0m\n");
            } 
            else {
                is_logged_in = false;
                printHeading("\033[3mLogged out successfully.\033[0m\n");
            }
            break;

        case '0': // '0' to exit the program
            printf("Exiting...\n");
            mysql_close(conn);
            exit(0);

        default:
            printHeading("\033[3mInvalid option. Please try again.\033[0m\n");
            break;
        }
    }

    return 0;
}