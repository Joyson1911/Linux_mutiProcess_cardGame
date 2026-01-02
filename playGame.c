#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int calCardValue(char[3]); // function prototype
int main(int argc, char *argv[])
{
    int i = 0, j, z = 0; // i = counter of cards, j = temp counter for playerNum, z = counter of child_cards
    int n, m;
    char cards[100][3];
    int returnpid = 1; // initialization of returnpid
    int playerNum = atoi(argv[1]);
    int fd[playerNum][2][2]; // 0 = parent to child, 1 = child to parent
    int pid;

    int childID = -1;
    char terminator[3] = {'\0','\0','\0'};

    // read txt and put cards into array
    while (scanf("%2s", cards[i]) != EOF)
        i++;

    for (j = 0; j <= playerNum; j++)
    {
        if (returnpid == 0) // child processs get their childID
        {
            if (childID == -1)
                childID = j;
            break;
        }
        // fork process
        else if (returnpid > 0 && j != playerNum) // only parent process can fork and create pipe
        {
            // create pipe for parent and child commumication
            if (pipe(fd[j][0]) < 0)
            {
                printf("Pipe creation error\n");
                exit(1);
            }
            if (pipe(fd[j][1]) < 0)
            {
                printf("Pipe creation error\n");
                exit(1);
            }
            returnpid = fork();
        }
    }

    pid = getpid();

    if (returnpid < 0) // fork error
    {
        printf("Fork failed\n");
        exit(1);
    }

    else if (returnpid == 0) // child
    {
        int status = 0; // 0 = child not completed, 1 = child completed
        char child_cards[20][3];
        char buff[3];
        int flag = -1;
        char playCard[3] = "S2";
        int cardNum;

        close(fd[childID - 1][1][0]); // close child to parent in
        close(fd[childID - 1][0][1]); // close parent to child out

        write(fd[childID - 1][1][1], &pid, sizeof(pid)); // child pass its pid to parent

        while (read(fd[childID - 1][0][0], child_cards[z], 3) > 0 && strcmp(child_cards[z], terminator) != 0)
        { // child wait to receive cards from parnet
            z++;
        }
        cardNum = z;
        printf("Child %d, %d: I have %d cards\n", childID, pid, z);
        printf("Child %d, %d: ", childID, pid);
        for (n = 0; n < z; n++)
            printf("%2s ", child_cards[n]);
        printf("\n");

        while (read(fd[childID - 1][0][0], buff, 3) > 0)
        {
            flag = -1;
            strcpy(playCard, "S2");
            for (n = 0; n < z; n++)
            {
                // find out smallest card that is just larger than the previous played card
                if (calCardValue(buff) < calCardValue(child_cards[n]) && calCardValue(child_cards[n]) <= calCardValue(playCard))
                {
                    strcpy(playCard, child_cards[n]);
                    flag = n;
                }
            }
            if (flag == -1)
            {
                // send a "pass" call to parent
                printf("Child %d: pass\n", childID);
                strcpy(playCard, "ps");
                write(fd[childID - 1][1][1], playCard, 3);
            }
            else
            {
                // play the larger card
                printf("Child %d: play %s\n", childID, playCard);
                write(fd[childID - 1][1][1], playCard, 3);
                strcpy(child_cards[flag], "00");
                cardNum--;
            }

            // pass the status of whether child complete
            if (cardNum != 0)
                write(fd[childID - 1][1][1], &status, 3);
            else
            {
                // send a "completion" call to parent
                status = 1;
                printf("Child %d: I complete\n", childID);
                write(fd[childID - 1][1][1], &status, sizeof(int));
                break;
            }
        }

        close(fd[childID - 1][1][1]); // close child to parent out
        close(fd[childID - 1][0][0]); // close parent to child in
    }

    else // parent
    {
        int currentPlayer;
        int passCounter = 0; // counter of how many children have passed
        char temp[3];        // temporary storage for the previous card.
        char previous[3] = "01";
        int child_status[playerNum]; // 0 = child not completed, 1 = child completed
        int completionFlag = 0;
        memset(child_status, 0, sizeof(child_status)); // initialization of child status

        for (j = 0; j < playerNum; j++)
        {
            close(fd[j][1][1]); // close child to parent out
            close(fd[j][0][0]); // close parent to child in
        }
        // print out the children's pid
        j = 0;
        printf("Parent: the child players are ");
        while (read(fd[j][1][0], &pid, sizeof(pid)) > 0 && j < playerNum)
        {
            printf("%d ", pid);
            j++;
        }
        printf("\n");

        // remove duplicate
        for (n = 0; n < i; n++)
        {
            for (m = 0; m < i; m++)
            {
                if (n == m || strcmp(cards[m], "00") == 0)
                    continue;
                if (calCardValue(cards[n]) == calCardValue(cards[m]))
                {
                    printf("Parent: duplicated card %s is discarded\n", cards[m]);
                    strcpy(cards[m], "00");
                }
            }
        }

        // distribute the cards to children
        j = 0;
        for (n = 0; n < i; n++)
        {
            if (strcmp(cards[n], "00") == 0)
                continue;
            if (calCardValue(cards[n]) == 30)
                currentPlayer = j;
            write(fd[j][0][1], cards[n], 3);
            if (j == (playerNum - 1))
                j = 0;
            else
                j++;
        }
        for (j = 0; j < playerNum; j++)
        {
            write(fd[j][0][1], terminator, 3);
        }

        while (j != 1)
        {
            // everyone else passes, the last player of the previously can play any card
            if (completionFlag == 0)
            {
                if (strcmp(temp, previous) == 0 && passCounter == (j - 1))
                {
                    strcpy(previous, "00");
                    passCounter = 0;
                }
            }
            // no one can play higher than the finisher; the player next to the finishing player play the smallest card
            else
            {
                if (strcmp(temp, previous) == 0 && passCounter == j)
                {
                    strcpy(previous, "00");
                    passCounter = 0;
                    completionFlag = 0;
                }
            }
            // relay the card to the current player for its consideration
            write(fd[currentPlayer][0][1], previous, 3);
            strcpy(temp, previous);
            // receive "pass" call or the card played by the current player
            while (read(fd[currentPlayer][1][0], previous, 3) > 0)
            {
                if (strcmp(previous, "ps") == 0)
                {
                    printf("Parent: child %d passes\n", currentPlayer + 1);
                    passCounter++;
                    strcpy(previous, temp);
                    break;
                }
                else
                {
                    printf("Parent: child %d play %2s\n", currentPlayer + 1, previous);
                    passCounter = 0;
                    completionFlag = 0;
                    break;
                }
            }

            // gain child status
            while (read(fd[currentPlayer][1][0], &child_status[currentPlayer], sizeof(int)) > 0)
            {
                if (child_status[currentPlayer] == 1)
                {
                    completionFlag = 1;
                    if (j == playerNum)
                        printf("Parent: child %d is winnner\n", currentPlayer + 1); // winner announcetment
                    else
                        printf("Parent: child %d completes\n", currentPlayer + 1);
                    j--;
                    break;
                }
                else
                    break;
            }

            // find out the next player
            while (j != 1)
            {
                if (currentPlayer == (playerNum - 1))
                    currentPlayer = 0;
                else
                    currentPlayer++;

                if (child_status[currentPlayer] == 0)
                    break;
            }
        }

        for (j = 0; j < playerNum; j++)
        {
            close(fd[j][1][0]); // close child to parent in
            close(fd[j][0][1]); // close parent to child out
        }
        wait(NULL);
        // loser announcetment
        for (n = 0; n < (sizeof(child_status) / sizeof(int)); n++)
        {
            if (child_status[n] == 0)
                printf("Parent: Child %d is loser\n", n + 1);
        }
    }
}

int calCardValue(char card[3])
{
    int card_value;

    switch (card[1])
    {
    case '2':
        card_value = 150;
        break;
    case 'A':
        card_value = 140;
        break;
    case 'K':
        card_value = 130;
        break;
    case 'Q':
        card_value = 120;
        break;
    case 'J':
        card_value = 110;
        break;
    case 'T':
        card_value = 100;
        break;
    default:
        card_value = (card[1] - 48) * 10;
        break;
    }

    switch (card[0])
    {
    case 'S':
        card_value += 3;
        break;
    case 'H':
        card_value += 2;
        break;
    case 'C':
        card_value += 1;
        break;
    default: // 'D' +=0
        break;
    }

    return card_value;
}