#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char policy[10];
int M;//virtual page number
int N;//physical frame number
int *vp;
int **vp_esca;//[1] 0->read 1->write
int *disk;
char state[10];
int evict = -1;
int destination = -1;
int source = -1;
int PFI = 0;
int physical_number = 0;
int fifo_oldest = 0;
int *fifo_physical;
int *esca_physical;
int *slru_physical;
int **replace_esca;
int esca_index = -1;
int *reference;
int *active_bit;
int inactive_num = 0;
int active_num = 0;
int INACTIVE_NUM;
int ACTIVE_NUM;
double page_fault = 0;
static int m;//number of instruction
static char *inputfile,*outputfile;
void readfile();
void getvpindex();
void getindex();
void FIFO();
int find_minIndex_disk();
void ESCA();
int esca_replace();
int find00();
int find01();
void SLRU();
void push_inactive_front(int vp);
void push_active_front(int vp);
void delete_inactive(int vp);
void delete_active(int vp);
int inactive_replace();
void active_replace();
int search_physical_index();
struct Ptable
{
    int present;
    int inuse;
    int index;
};
struct slru
{
    int page;
    struct slru *next;
};
struct Ptable *page_table;
struct slru *active_head,*active_tail, *inactive_head, *inactive_tail;

int main(int argc,char *argv[])
{

    inputfile = argv[1];
    outputfile = argv[2];

    readfile();
    disk = malloc(M*sizeof(int));

    page_table = (struct Ptable*)malloc(M*sizeof(struct Ptable));

    //printf("%s\n",policy);
    //printf("M:%d\n",M);
    //printf("N:%d\n",N);
    for(int j=0; j<M; j++)
    {
        page_table[j].present = 0;
        page_table[j].inuse = 0;
        page_table[j].index = -1;
        disk[j] = -1;
    }
    //printf("%s\n",policy);
    if(strcmp(policy,"FIFO") == 0)
    {
        fifo_physical = malloc(N*sizeof(int));
        vp = malloc(m*sizeof(int));
        getvpindex();
        FIFO();

    }
    else if(strcmp(policy,"ESCA") == 0)
    {
        esca_physical = malloc(N*sizeof(int));
        vp_esca = (int**)malloc(m*sizeof(int*));
        replace_esca = (int**)malloc(N*sizeof(int*));
        for(int i=0; i<N; i++)
            replace_esca[i] = (int*)malloc(sizeof(int)*2);
        for(int i=0; i<N; i++)
        {
            for(int j=0; j<2; j++)
                replace_esca[i][j] = -1;
        }
        for(int i=0; i<m; i++)
            vp_esca[i] = (int*)malloc(sizeof(int)*2);
        getindex();
        //for(int i=0;i<m;i++)
        //    printf("%d %d\n",vp_esca[i][1],vp_esca[i][0]);
        ESCA();

    }
    else if(strcmp(policy,"SLRU") == 0)
    {
        vp = malloc(m*sizeof(int));
        slru_physical = malloc(N*sizeof(int));
        getvpindex();
        for(int i = 0; i<N; i++)
            slru_physical[i] = -1;
        reference = malloc(N*sizeof(int));
        active_bit = malloc(N*sizeof(int));
        INACTIVE_NUM = (N+1)/2;
        ACTIVE_NUM = N-INACTIVE_NUM;
        //printf("%d, %d\n",INACTIVE_NUM,ACTIVE_NUM);
        struct slru *inactive_head = (struct slru*)malloc(sizeof(struct slru));
        struct slru *inactive_tail = (struct slru*)malloc(sizeof(struct slru));
        struct slru *active_head = (struct slru*)malloc(sizeof(struct slru));
        struct slru *active_tail = (struct slru*)malloc(sizeof(struct slru));
        /*for(int i=0;i<N;i++){
            reference[i] = -1;
            active_bit[i] = -1;
        }*/
        SLRU();
    }


    return  0;
}
void SLRU()
{
    FILE *pout;
    pout = fopen(outputfile,"wb");
    char output_string[100];


    //push_inactive_front(1);
    //push_inactive_front(2);
    //printf("%d\n",inactive_head->page);
    //delete_inactive(2);
    //printf("%d\n",inactive_head->page);
    for(int i=0; i<m; i++)
    {
        memset(output_string,'\0',100);
        memset(state,'\0',10);
        //printf("i:%d\n",vp[i]);
        if(page_table[vp[i]].present == 0)//miss
        {
            strncpy(state,"Miss",strlen("Miss"));
            page_fault++;
            if(physical_number <= N)
            {

                if(inactive_num < INACTIVE_NUM) //inactive not full
                {
                    //printf("d\n");
                    slru_physical[physical_number] = vp[i];
                    page_table[vp[i]].present = 1;
                    page_table[vp[i]].inuse = 1;
                    PFI = physical_number;
                    evict = -1;
                    source = -1;
                    //source = page_table[vp[i]].index;
                    //disk[source] = -1;
                    destination = -1;
                    push_inactive_front(vp[i]);
                    page_table[vp[i]].index = physical_number;
                    reference[vp[i]] = 1;
                    active_bit[vp[i]] = 0;
                    //printf("%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                    sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                    fwrite(output_string,1,strlen(output_string),pout);
                    //printf("inactive num:%d\n",inactive_num);
                }
                else if(inactive_num == INACTIVE_NUM) //inactive full
                {
                    //printf("in\n");

                    PFI = inactive_replace();
                    source = page_table[vp[i]].index;
                    disk[source] = -1;
                    page_table[vp[i]].index = PFI;
                    slru_physical[page_table[vp[i]].index] = vp[i];
                    page_table[vp[i]].present = 1;
                    page_table[vp[i]].inuse = 1;

                    push_inactive_front(vp[i]);
                    reference[vp[i]] = 1;
                    active_bit[vp[i]] = 0;
                    //printf("%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                    sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                    fwrite(output_string,1,strlen(output_string),pout);
                }
                if(physical_number < N-1)
                    physical_number++;

            }

        }
        else if(page_table[vp[i]].present == 1)//hit
        {
            if(active_bit[vp[i]] == 0)//inactive
            {
                if(reference[vp[i]] == 1 && ACTIVE_NUM != 0)
                {
                    if(active_num == ACTIVE_NUM)
                    {
                        //printf("active full\n");
                        delete_inactive(vp[i]);
                        active_replace();
                        push_active_front(vp[i]);
                        reference[vp[i]] = 0;
                        active_bit[vp[i]] = 1;
                        PFI = page_table[vp[i]].index;
                        //printf("f\n");
                    }
                    else if(active_num < ACTIVE_NUM)
                    {
                        PFI = page_table[vp[i]].index;
                        reference[vp[i]] = 0;
                        active_bit[vp[i]] = 1;
                        delete_inactive(vp[i]);
                        push_active_front(vp[i]);

                    }
                }
                else
                {
                    PFI = page_table[vp[i]].index;
                    reference[vp[i]] = 1;
                    delete_inactive(vp[i]);
                    push_inactive_front(vp[i]);
                    active_bit[vp[i]] = 0;
                }
            }
            else if(active_bit[vp[i]] == 1) //active
            {
                PFI = page_table[vp[i]].index;
                reference[vp[i]] = 1;
                delete_active(vp[i]);
                push_active_front(vp[i]);
            }
            strncpy(state,"Hit",strlen("Hit"));
            sprintf(output_string,"%s, %d=>%d\n",state,vp[i],PFI);
            fwrite(output_string,1,strlen(output_string),pout);
        }
    }
    memset(output_string,'\0',100);
    double page_fault_rate = (page_fault/m);
    sprintf(output_string,"Page Fault Rate: %.3f",page_fault_rate);
    fwrite(output_string,1,strlen(output_string),pout);
    fclose(pout);
}
int search_physical_index()
{
    for(int i=0; i<N; i++)
    {
        if(slru_physical[i] == -1)
        {
            return i;
        }
    }
}
void active_replace()
{
    //printf("active tail:%d\n",active_tail->page);
    int victim = -1;
    int tail = active_tail->page;
    int index = -1;
    while(victim == -1)
    {
        tail = active_tail->page;
        if(reference[tail] == 0)
        {

            victim = tail;
            if(inactive_num == INACTIVE_NUM)//inactive is full
            {
                PFI = inactive_replace();
            }
            delete_active(victim);
            push_inactive_front(victim);
            active_bit[victim] = 0;
            break;
        }
        else
        {

            reference[tail] = 0;
            delete_active(tail);
            push_active_front(tail);
        }
    }
    //PFI = page_table[victim].index;
}
int inactive_replace()//swap out
{
    //printf("inactive tail:%d\n",inactive_tail->page);
    int victim = -1;
    int tail = inactive_tail->page;
    int index = -1;
    while(victim == -1)
    {
        tail = inactive_tail->page;
        if(reference[tail] == 0)
        {
            //printf("tail:%d, reference:%d\n",tail,reference[tail]);
            victim = tail;
            break;
        }
        else if(reference[tail] == 1)
        {
            //printf("tail:%d, reference:%d\n",tail,reference[tail]);
            reference[tail] = 0;
            delete_inactive(tail);
            push_inactive_front(tail);
        }

    }
    //printf("victim:%d\n",victim);
    delete_inactive(victim);
    destination = find_minIndex_disk();
    evict = victim;
    //printf("evict:%d\n",evict);
    page_table[evict].present = 0;
    disk[destination] = evict;
    PFI = page_table[evict].index;
    page_table[evict].index = destination;
    slru_physical[PFI] = -1;
    reference[evict] = -1;
    active_bit[evict] = -1;
    physical_number--;
    //inactive_num--;
    return PFI;

}
void push_inactive_front(int vp)
{
    //printf("%d\n",inactive_head);
    struct slru *node = (struct slru*)malloc(sizeof(struct slru));
    node->page = vp;
    node->next = inactive_head;
    inactive_head = node;
    if(inactive_num == 0)
    {
        //printf("in\n");
        inactive_tail = node;
        inactive_tail->next = NULL;
    }
    //printf("inactive tail:%d\n",inactive_tail->page);
    inactive_num++;
}
void push_active_front(int vp)
{
    struct slru *node = (struct slru*)malloc(sizeof(struct slru));
    node->page = vp;
    node->next = active_head;
    active_head = node;
    if(active_num == 0)
    {
        //printf("in\n");
        active_tail = node;
        active_tail->next = NULL;
    }
    active_num++;
}
void delete_inactive(int vp)
{
    struct slru *current = (struct slru*)malloc(sizeof(struct slru));
    struct slru *previous = (struct slru*)malloc(sizeof(struct slru));
    current = inactive_head;
    previous = 0;
    while(current != 0 && current->page != vp)
    {
        previous = current;
        current = current->next;
    }

    if(current == inactive_head)
    {
        inactive_head = current->next;
        free(current);
        current = 0;
    }
    else if(current == inactive_tail)
    {
        previous->next = NULL;
        inactive_tail = previous;
        free(current);
        current = 0;
    }
    else
    {
        previous->next = current->next;
        free(current);
        current = 0;
    }
    inactive_num--;

}
void delete_active(int vp)
{
    struct slru *current = (struct slru*)malloc(sizeof(struct slru));
    struct slru *previous = (struct slru*)malloc(sizeof(struct slru));
    current = active_head;
    previous = 0;
    while(current != 0 && current->page != vp)
    {
        previous = current;
        current = current->next;
    }

    if(current == active_head)
    {
        active_head = current->next;
        free(current);
        current = 0;
    }
    else if(current == active_tail)
    {
        previous->next = NULL;
        active_tail = previous;
        free(current);
        current = 0;
    }
    else
    {
        previous->next = current->next;
        free(current);
        current = 0;
    }
    active_num--;
}
void ESCA()
{
    FILE *pout;
    pout = fopen(outputfile,"wb");
    char output_string[100];
    physical_number = 0;
    page_fault = 0;
    for(int i=0; i<m; i++)
    {
        memset(output_string,'\0',100);
        memset(state,'\0',10);
        if(page_table[vp_esca[i][0]].present == 0)
        {
            //printf("MISS\n");
            page_fault++;
            strncpy(state,"Miss",strlen("Miss"));
            if(physical_number < N)
            {
                esca_physical[physical_number] = vp_esca[i][0];
                page_table[vp_esca[i][0]].present = 1;
                page_table[vp_esca[i][0]].inuse = 1;
                page_table[vp_esca[i][0]].index = physical_number;
                PFI = physical_number;
                evict = -1;
                source = -1;
                destination = -1;
                if(vp_esca[i][1] == 0)//read
                    replace_esca[physical_number][1] = 0;
                else//write
                    replace_esca[physical_number][1] = 1;
                replace_esca[physical_number][0] = 1;
                //printf("%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp_esca[i][0],source);
                sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp_esca[i][0],source);
                fwrite(output_string,1,strlen(output_string),pout);
                physical_number++;
            }
            else if(physical_number == N)/*full*/
            {
                destination = find_minIndex_disk();
                evict = esca_replace();
                //printf("evict:%d\n",evict);
                page_table[evict].present = 0;
                disk[destination] = evict;
                page_table[evict].index = destination;
                source = page_table[vp_esca[i][0]].index;
                if(source != -1)
                    disk[source] = -1;
                PFI = esca_index;
                if(vp_esca[i][1] == 0)//read
                    replace_esca[esca_index][1] = 0;
                else//write
                    replace_esca[esca_index][1] = 1;
                replace_esca[esca_index][0] = 1;
                esca_physical[esca_index] = vp_esca[i][0];
                page_table[vp_esca[i][0]].present = 1;
                page_table[vp_esca[i][0]].inuse = 1;
                page_table[vp_esca[i][0]].index = PFI;
                sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp_esca[i][0],source);
                fwrite(output_string,1,strlen(output_string),pout);
            }
        }
        else if(page_table[vp_esca[i][0]].present == 1)
        {
            //printf("HIT\n");
            if(vp_esca[i][1] == 0)//read
                replace_esca[page_table[vp_esca[i][0]].index][1] = 0;
            else//write
                replace_esca[page_table[vp_esca[i][0]].index][1] = 1;
            replace_esca[page_table[vp_esca[i][0]].index][0] = 1;
            strncpy(state,"Hit",strlen("Hit"));
            sprintf(output_string,"%s, %d=>%d\n",state,vp_esca[i][0],page_table[vp_esca[i][0]].index);
            fwrite(output_string,1,strlen(output_string),pout);
        }

    }
    memset(output_string,'\0',100);
    double page_fault_rate = (page_fault/m);
    sprintf(output_string,"Page Fault Rate: %.3f",page_fault_rate);
    fwrite(output_string,1,strlen(output_string),pout);
    fclose(pout);
}
int esca_replace()
{
    int victim = 0;
    while(1)
    {
        victim = find00();
        if(victim != -1)
        {
            return victim;
        }
        else if(victim == -1)
        {
            victim = find01();
            if(victim != -1)
                return victim;
            else
                continue;
        }
    }
}
int find00()
{
    for(int i=0; i<N; i++)
    {
        if(replace_esca[i][0] == 0 && replace_esca[i][1] == 0)//00
        {
            esca_index = i;
            return esca_physical[i];
        }
    }
    return -1;
}
int find01()
{
    for(int i=0; i<N; i++)
    {
        if(replace_esca[i][0] == 0 && replace_esca[i][1] == 1)
        {
            esca_index = i;
            return esca_physical[i];
        }
        else
        {
            replace_esca[i][0] = 0;
        }
    }
    return -1;
}
void FIFO()
{
    FILE *pout;
    pout = fopen(outputfile,"wb");
    char output_string[100];

    for(int i=0; i<m; i++)
    {
        memset(output_string,'\0',100);
        memset(state,'\0',10);
        if(page_table[vp[i]].present == 0)
        {
            strncpy(state,"Miss",strlen("Miss"));
            page_fault++;
            if(physical_number < N)/*empty*/
            {
                fifo_oldest = 0;
                fifo_physical[physical_number] = vp[i];
                page_table[vp[i]].present = 1;
                page_table[vp[i]].inuse = 1;
                page_table[vp[i]].index = physical_number;
                PFI = physical_number;
                evict = -1;
                source = -1;
                destination = -1;
                sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                fwrite(output_string,1,strlen(output_string),pout);
                physical_number++;
            }
            else if(physical_number == N)/*full*/
            {
                destination = find_minIndex_disk();
                //printf("des:%d\n",destination);
                evict = fifo_physical[fifo_oldest];
                page_table[evict].present = 0;
                disk[destination] = evict;
                page_table[evict].index = destination;
                source = page_table[vp[i]].index;//!=-1 it is already in disk
                //printf("source:%d\n",source);
                if(source != -1)
                    disk[source] = -1;
                PFI = fifo_oldest;
                fifo_physical[PFI] = vp[i];


                if(fifo_oldest == N-1)
                    fifo_oldest = 0;
                else
                    fifo_oldest++;

                fifo_physical[PFI] = vp[i];
                page_table[vp[i]].present = 1;
                page_table[vp[i]].inuse = 1;
                page_table[vp[i]].index = PFI;


                sprintf(output_string,"%s, %d, %d>>%d, %d<<%d\n",state,PFI,evict,destination,vp[i],source);
                fwrite(output_string,1,strlen(output_string),pout);
            }
        }
        else if(page_table[vp[i]].present == 1)
        {
            strncpy(state,"Hit",strlen("Hit"));
            sprintf(output_string,"%s, %d=>%d\n",state,vp[i],page_table[vp[i]].index);
            fwrite(output_string,1,strlen(output_string),pout);
        }
    }
    memset(output_string,'\0',100);
    double page_fault_rate = (page_fault/m);
    sprintf(output_string,"Page Fault Rate: %.3f",page_fault_rate);
    fwrite(output_string,1,strlen(output_string),pout);
    fclose(pout);
}

void getindex()
{
    FILE *fp;
    fp = fopen(inputfile,"r");
    int i=0;
    char ch[60];
    while(fgets(ch,60,fp) != NULL)
    {
        if(i>3)
        {
            //printf("%s",ch);
            char *delim = ": \n";
            char *str;
            char *num;
            str = strtok(ch,delim);
            //printf("%s ",str);
            if(strcmp(str,"Write") == 0)
                vp_esca[i-4][1] = 1;
            else
                vp_esca[i-4][1] = 0;
            num = strtok(NULL,delim);
            //printf("%s\n",num);
            vp_esca[i-4][0] = atoi(num);
        }

        i++;
    }
    fclose(fp);

}
int find_minIndex_disk()
{
    int index = -1;
    for(int i=0; i<M; i++)
    {
        if(disk[i] == -1)
        {
            index = i;
            return index;
        }
    }
}
void readfile()
{
    FILE *fp;
    fp = fopen(inputfile,"r");
    if(fp == NULL)
    {
        printf("failed to open file\n");
    }
    char c[3][60];
    int l=0;
    char ch[60];
    while(fgets(ch,60,fp) != NULL)
    {
        if(l<3)
        {
            strncpy(c[l],ch,strlen(ch));
            //c[l] = ch;
            //fgets(c[l],60,fp);
        }

        l++;
    }
    m = l-4;

    char *delim = ": \n";
    char *str;
    str = strtok(c[0],delim);
    str = strtok(NULL,delim);
    strncpy(policy,str,strlen(str));
    for(int i=1; i<3; i++)
    {
        str = strtok(c[i],delim);
        int j = 0;
        while(str != NULL)
        {
            j++;
            str = strtok(NULL,delim);
            if(j == 4 && i == 1)
            {
                M = atoi(str);
            }
            else if(j == 4 && i == 2)
            {
                N = atoi(str);
            }
        }
    }
    fclose(fp);

}
void getvpindex()
{
    FILE *fp;
    fp = fopen(inputfile,"r");
    int i=0;
    char ch[60];
    while(fgets(ch,60,fp) != NULL)
    {
        if(i>3)
        {
            //printf("%s",ch);
            char *delim = ": \n";
            char *str;
            str = strtok(ch,delim);
            str = strtok(NULL,delim);
            vp[i-4] = atoi(str);
        }

        i++;
    }
    fclose(fp);

    //for(int j=0;j<m;j++)
    //    printf("%d\n",vp[j]);
}

